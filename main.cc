#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <termios.h>
#include <thread>
#include <unistd.h>
#include <vector>

using namespace std;

// Constantes globales y enums
#define BOARD_SIZE 8
#define TABLERO_X0 4
#define TABLERO_Y0 2
#define CELDA_W 3
#define CELDA_H 1
#define PROMPT_TIME 2000
#define MSG_SHORT 700
#define MSG_MEDIUM 1000
#define MSG_LONG 4000

// Colores fijos para las piezas por bando (alto contraste en fondos negro/blanco/verde)
#define FG_WHITE_PIECE "\033[38;5;196m"  // Rojo brillante para "blancas"
#define FG_BLACK_PIECE "\033[38;5;39m"   // Cian brillante para "negras"

#define BOARD_HEADER_LINES 1
#define BOARD_FOOTER_LINES 1
#define BOARD_TOTAL_LINES (BOARD_HEADER_LINES + BOARD_SIZE + BOARD_FOOTER_LINES)
#define STATUS_ROW (BOARD_TOTAL_LINES + 2)

// Enum para las piezas de ajedrez
enum PiezaTipo {
    VACIO,
    PEON,
    CABALLO,
    ALFIL,
    TORRE,
    REY,
    REINA
};

enum Color {
    COLOR_VACIO = -1,
    BLANCO = 0,
    NEGRO = 1
};

// Mapeo de PiezaTipo a Unicode (mismo modelo para ambos bandos; el color se da por el FG)
const char* PIEZA_UNICODE[7] = {" ", "♟", "♞", "♝", "♜", "♛", "♚"};

const char* COLUMNAS = "    A  B  C  D  E  F  G  H    \n";
struct Pieza {
    PiezaTipo tipo;
    Color color; 
    bool movido; // Se utiliza para el enroque y captura al paso
};

typedef vector<Pieza> VP;
typedef vector<VP> VVP;

typedef vector<bool> VB;
typedef vector<VB> VVB;

// Sincronización de salida para evitar intercalado en terminal
static std::mutex g_ioMutex;
static std::string g_statusText;
static std::atomic<long> g_statusSeq{0};
// En passant target square (row, col) valid only for the immediate reply move; -1 if none
static int g_epRow = -1;
static int g_epCol = -1;

// Utilidad: convierte coordenadas (fila i, columna j) a notación algebraica (e.g., "E4")
static inline string toAlg(int i, int j) {
    char file = 'A' + j;
    char rank = '0' + (BOARD_SIZE - i);
    string s; s += file; s += rank; return s;
}

// Utilidad: transponer una matriz booleana NxN
VVB transposePV(const VVB& src) {
    VVB dst(BOARD_SIZE, VB(BOARD_SIZE, false));
    for (int r = 0; r < BOARD_SIZE; ++r)
        for (int c = 0; c < BOARD_SIZE; ++c)
            dst[r][c] = src[c][r];
    return dst;
}

// Declaración adelantada
void imprimirMensaje(string s, int tiempo, bool paralelo);
void setStatusMessage(const string& s);
void clearStatusMessage();

// Activa/desactiva modo raw en la terminal
void setRawMode(bool enable) {
    static struct termios oldt, newt;
    if (enable) {
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    } else tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

// Lee un evento de ratón (xterm). Soporta legacy (1002) y SGR (1006).
// Devuelve i,j (celda) y flags de suelta/movimiento.
static inline bool readByte(unsigned char& ch) {
    int r = read(STDIN_FILENO, &ch, 1);
    return r == 1;
}

bool leerEventoRaton(int& i, int& j, bool& isRelease, bool& isMotion) {
    unsigned char b0;
    if (!readByte(b0)) return false;
    if (b0 != 0x1B) return false; // ESC
    unsigned char b1;
    if (!readByte(b1) || b1 != '[') return false; // CSI
    unsigned char b2;
    if (!readByte(b2)) return false;

    int cb = 0, x = 0, y = 0; // coordenadas 1-based de terminal
    if (b2 == 'M') {
        // Legacy: ESC [ M cb cx cy (todos: byte + 32)
        unsigned char cbb, cxb, cyb;
        if (!readByte(cbb) || !readByte(cxb) || !readByte(cyb)) return false;
        cb = int(cbb) - 32;
        x = int(cxb) - 32;
        y = int(cyb) - 32;
        isRelease = ((cb & 3) == 3);
        isMotion = ((cb & 32) != 0);
    } else if (b2 == '<') {
        // SGR: ESC [ < Cb ; Cx ; Cy (M or m)
        // Leer Cb
        int acc = 0; unsigned char ch;
        while (readByte(ch)) {
            if (ch >= '0' && ch <= '9') acc = acc*10 + (ch - '0');
            else if (ch == ';') { cb = acc; break; }
            else return false;
        }
        // Leer X
        acc = 0;
        while (readByte(ch)) {
            if (ch >= '0' && ch <= '9') acc = acc*10 + (ch - '0');
            else if (ch == ';') { x = acc; break; }
            else return false;
        }
        // Leer Y y terminador (M=press/motion, m=release)
        acc = 0; unsigned char trail = 0;
        while (readByte(ch)) {
            if (ch >= '0' && ch <= '9') acc = acc*10 + (ch - '0');
            else if (ch == 'M' || ch == 'm') { y = acc; trail = ch; break; }
            else return false;
        }
        isRelease = (trail == 'm');
        isMotion = ((cb & 32) != 0);
    } else {
        // Otras secuencias (p.ej., foco) no soportadas
        return false;
    }

    // Ajusta a tablero (asume tablero en la esquina superior)
    x -= TABLERO_X0;
    y -= TABLERO_Y0;
    if (x < 0 || y < 0) return false;
    // i = fila (row), j = columna (col) según uso interno
    i = y / CELDA_H;
    j = (x + 1) / CELDA_W;
    if (i < 0 || i >= BOARD_SIZE || j < 0 || j >= BOARD_SIZE) return false;
    return true;
}

// Helpers para gestionar el tracking del ratón
void habilitarTrackingRaton() {
    // Habilita seguimiento de botones y formato SGR para compatibilidad con pantallas táctiles
    cout << "\033[?1002h"; // Button-event tracking
    cout << "\033[?1006h"; // SGR mouse mode (recomendado por terminales modernos)
    cout.flush();
    setRawMode(true);
}

void deshabilitarTrackingRaton() {
    setRawMode(false);
    cout << "\033[?1006l"; // Disable SGR mouse
    cout << "\033[?1002l"; // Disable button tracking
    cout.flush();
}

// Devuelve la celda donde se presiona por primera vez (mouse down)
void obtenerOrigenRaton(int& i, int& j) {
    imprimirMensaje("Selecciona una pieza con el ratón", MSG_SHORT, true);
    habilitarTrackingRaton();
    while (true) {
        bool release = false, motion = false;
        int ci, cj;
        if (!leerEventoRaton(ci, cj, release, motion)) continue;
        if (!release) { // press
            i = ci; j = cj;
            return; // Mantiene tracking habilitado para el drag
        }
    }
}

// Tras un origen, devuelve destino: si se suelta en otra casilla => drag;
// si se suelta en la misma casilla => espera un segundo clic y suelta para el destino.
void obtenerDestinoTrasOrigen(int i0, int j0, int& i1, int& j1) {
    int last_i = i0, last_j = j0;
    // Espera release del primer clic/drag
    while (true) {
        bool release = false, motion = false;
        int ci, cj;
        if (!leerEventoRaton(ci, cj, release, motion)) continue;
        last_i = ci; last_j = cj;
        if (release) break;
    }
    // Si release en la misma casilla, pide segundo clic
    if (last_i == i0 && last_j == j0) {
    imprimirMensaje("Ahora selecciona la casilla de destino", MSG_SHORT, true);
        bool pressed2 = false;
        while (true) {
            bool release = false, motion = false;
            int ci, cj;
            if (!leerEventoRaton(ci, cj, release, motion)) continue;
            if (!pressed2 && !release) { // segundo press
                i1 = ci; j1 = cj; // opcional: mostrar hover
                pressed2 = true;
            }
            if (pressed2 && release) { // suelta del segundo clic
                i1 = ci; j1 = cj;
                break;
            }
        }
    } else {
        // Drag válido: usar la posición del release
        i1 = last_i; j1 = last_j;
    }
    deshabilitarTrackingRaton();
}

int abs(int a) {
    if (a < 0) return -a;
    return a;
}

bool entreLimites(int i, int j) {
    if (i < BOARD_SIZE and j < BOARD_SIZE and i >= 0 and j >= 0) return true;
    return false;
}

VVP crearTablero() {
    VVP Tablero(BOARD_SIZE, VP(BOARD_SIZE, Pieza{VACIO, COLOR_VACIO, false}));

    // Colores
    for (int i = 0; i < 2; ++i) for (int j = 0; j < BOARD_SIZE; ++j) Tablero[i][j].color = NEGRO;
    for (int i = 6; i < 8; ++i) for (int j = 0; j < BOARD_SIZE; ++j) Tablero[i][j].color = BLANCO;

    // Peones
    for (int i = 0; i < BOARD_SIZE; ++i) Tablero[1][i].tipo = PEON;
    for (int i = 0; i < BOARD_SIZE; ++i) Tablero[6][i].tipo = PEON;

    // Piezas principales
    Tablero[0][0].tipo = TORRE;
    Tablero[0][1].tipo = CABALLO;
    Tablero[0][2].tipo = ALFIL;
    Tablero[0][3].tipo = REINA;
    Tablero[0][4].tipo = REY;
    Tablero[0][5].tipo = ALFIL;
    Tablero[0][6].tipo = CABALLO;
    Tablero[0][7].tipo = TORRE;

    Tablero[7][0].tipo = TORRE;
    Tablero[7][1].tipo = CABALLO;
    Tablero[7][2].tipo = ALFIL;
    Tablero[7][3].tipo = REINA;
    Tablero[7][4].tipo = REY;
    Tablero[7][5].tipo = ALFIL;
    Tablero[7][6].tipo = CABALLO;
    Tablero[7][7].tipo = TORRE;

    return Tablero;
}

// Mueve el cursor a la parte superior izquierda
void moveCursorHome() {
    cout << "\033[H";
}

// Oculta el cursor
void hideCursor() {
    cout << "\033[?25l";
}

// Muestra el cursor
void showCursor() {
    cout << "\033[?25h";
}

// Mueve el cursor al inicio de la línea
void moveCursorLineStart() {
    cout << "\r";
}

// Mueve el cursor a una fila/columna concreta (1-indexed)
void moveCursorTo(int row, int col) {
    cout << "\033[" << row << ";" << col << "H";
}

// Dibuja el tablero con patrón ajedrez (negro/blanco). Si overlayPV se proporciona,
// resalta esas celdas con fondo verde.
void dibujarTablero(const VVP& Tablero, const VVB* overlayPV = nullptr) {
    std::lock_guard<std::mutex> lock(g_ioMutex);
    cout << "\033[2J\033[H";
    hideCursor();
    // Fondo marrón para bordes (256-colors) y texto claro para contraste
    const char* BORD_BG = "\033[48;5;94m"; // fondo marrón
    const char* BORD_FG = "\033[97m";     // texto claro
    const char* RESET = "\033[0m";
    cout << BORD_BG << BORD_FG << COLUMNAS << RESET;
    for (int i = 0; i < BOARD_SIZE; ++i) {
        cout << BORD_BG << BORD_FG << " " << BOARD_SIZE - i << " " << RESET;
        for (int j = 0; j < BOARD_SIZE; ++j) {
            const bool highlight = (overlayPV && (*overlayPV)[i][j]);
            const bool dark = ((i + j) % 2) == 1;
            const char* bg = highlight ? "\033[42m" : (dark ? "\033[40m" : "\033[47m");
            // Color del texto: piezas con color fijo por bando, siempre distinguible del fondo
            const char* fg;
            if (Tablero[i][j].tipo == VACIO) {
                // Para casillas vacías, mantener contraste simple con el fondo
                fg = highlight ? "\033[30m" : (dark ? "\033[97m" : "\033[30m");
            } else {
                fg = (Tablero[i][j].color == BLANCO ? FG_WHITE_PIECE : FG_BLACK_PIECE);
            }
            int tipo = Tablero[i][j].tipo;
            cout << bg << fg << " " << PIEZA_UNICODE[tipo] << " " << RESET;
        }
        cout << BORD_BG << BORD_FG << " " << BOARD_SIZE - i << " " << RESET << "\n";
    }
    cout << BORD_BG << BORD_FG << COLUMNAS << RESET;
    moveCursorTo(STATUS_ROW, 1);
    cout << "\033[2K";
    if (!g_statusText.empty()) cout << g_statusText;
    cout << flush;
    showCursor();
}

void imprimirTablero(const VVP& Tablero) {
    dibujarTablero(Tablero);
}

// Muestra un mensaje por tiempo milisegundos y despues lo elimina
void imprimirMensaje(string s, int tiempo, bool paralelo = true) {
    // Sanitiza: evita saltos de línea en la línea de estado
    for (char& c : s) if (c == '\n') c = ' ';
    long mySeq = ++g_statusSeq;
    auto imprimir_y_borrar = [s, tiempo, mySeq]() {
        {
            std::lock_guard<std::mutex> lock(g_ioMutex);
            g_statusText = s;
            moveCursorTo(STATUS_ROW, 1);
            // Borra toda la línea y escribe el mensaje
            cout << "\033[2K" << g_statusText << flush;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(tiempo));
        {
            std::lock_guard<std::mutex> lock(g_ioMutex);
            // Solo borra si nadie escribió un mensaje más nuevo
            if (mySeq == g_statusSeq.load()) {
                g_statusText.clear();
                moveCursorTo(STATUS_ROW, 1);
                cout << "\033[2K" << flush;
            }
        }
    };
    if (paralelo) std::thread(imprimir_y_borrar).detach();
    else imprimir_y_borrar();
}

// Escribe un mensaje fijo en la línea de estado (no se borra automáticamente)
void setStatusMessage(const string& s) {
    std::lock_guard<std::mutex> lock(g_ioMutex);
    g_statusText = s;
    moveCursorTo(STATUS_ROW, 1);
    cout << "\033[2K" << g_statusText << flush;
}

// Limpia la línea de estado si coincide con el último mensaje
void clearStatusMessage() {
    std::lock_guard<std::mutex> lock(g_ioMutex);
    g_statusText.clear();
    moveCursorTo(STATUS_ROW, 1);
    cout << "\033[2K" << flush;
}

char coronarPeones() {
    // Coronar peones
    setStatusMessage("Coronación: elige pieza (Q=Dama, R=Torre, B=Alfil, N=Caballo) y pulsa Enter: ");
    char corona;
    cin >> corona;
    while (true) {
        if (corona >= 'a' && corona <= 'z') corona = char(corona - 'a' + 'A');
        if (corona == 'Q' || corona == 'R' || corona == 'B' || corona == 'N') break;
        setStatusMessage("Entrada no válida. Usa Q, R, B o N y pulsa Enter: ");
        cin >> corona;
    }
    clearStatusMessage();
    imprimirMensaje(string("Coronado a ") + (corona=='Q'?"Dama": corona=='R'?"Torre": corona=='B'?"Alfil":"Caballo"), MSG_MEDIUM);
    return corona;
}

// Comprueba si la captura al paso es legal para el peón actual
bool enPassant(int colDest, int rowDest, PiezaTipo prevPiece, int prevRow, int prevCol, int currRow, int color) {
    // La pieza movida en la jugada anterior debe ser un peón
    if (prevPiece != PEON) return false;
    // El peón rival debe haber avanzado dos filas en la jugada anterior
    if (abs(currRow - prevRow) != 2) return false;
    // La captura al paso solo es posible en la quinta fila para blancas y cuarta para negras
    if (color == 0 && rowDest != 3) return false;
    if (color == 1 && rowDest != 4) return false;
    // El peón que intenta capturar debe estar en la misma columna que el peón rival
    if (prevCol != colDest) return false;
    // Si todas las condiciones se cumplen, la captura al paso es legal
    return true;
}
// Busca en vertical, horizontal y diagonales dependiendo de los valores de aux
// Pre: aux puede valer 1, -1 o 0
bool apunta(const VVP& Tablero, int i, int j, int color, int aux1, int aux2, PiezaTipo pieza) {
    int k = 1;
    while (entreLimites(i + k*aux1, j + k*aux2) and Tablero[i + k*aux1][j + k*aux2].color != color) {
        if (Tablero[i + k*aux1][j + k*aux2].tipo == pieza) return true;
        // Comprueba al mismo tiempo que no haya una reina
        if (Tablero[i + k*aux1][j + k*aux2].tipo == REINA) return true;
        if (Tablero[i + k*aux1][j + k*aux2].tipo != VACIO) break;
        ++k;
    }
    return false;
}

// Comprueba que no haya ningun alfil apuntando a esa casilla
bool apuntaAlfil(const VVP& Tablero, int i, int j, int color) {
    if (apunta(Tablero, i, j, color, 1, 1, ALFIL)) return true;
    if (apunta(Tablero, i, j, color, 1, -1, ALFIL)) return true;
    if (apunta(Tablero, i, j, color, -1, 1, ALFIL)) return true;
    if (apunta(Tablero, i, j, color, -1, -1, ALFIL)) return true;
    return false;
}

// Comprueba que no haya ninguna torre apuntando a esa casilla
bool apuntaTorre(const VVP& Tablero, int i, int j, int color) {
    if (apunta(Tablero, i, j, color, 1, 0, TORRE)) return true;
    if (apunta(Tablero, i, j, color, -1, 0, TORRE)) return true;
    if (apunta(Tablero, i, j, color, 0, 1, TORRE)) return true;
    if (apunta(Tablero, i, j, color, 0, -1, TORRE)) return true;
    return false;
}

// Comprueba que no haya ningun caballo apuntando a esa casilla
bool apuntaCaballo(const VVP& Tablero, int i, int j, int color) {
    if (entreLimites(i + 2, j + 1) and Tablero[i + 2][j + 1].tipo == CABALLO and Tablero[i + 2][j + 1].color != color) return true;
    if (entreLimites(i + 2, j - 1) and Tablero[i + 2][j - 1].tipo == CABALLO and Tablero[i + 2][j - 1].color != color) return true;
    if (entreLimites(i - 2, j + 1) and Tablero[i - 2][j + 1].tipo == CABALLO and Tablero[i - 2][j + 1].color != color) return true;
    if (entreLimites(i - 2, j - 1) and Tablero[i - 2][j - 1].tipo == CABALLO and Tablero[i - 2][j - 1].color != color) return true;
    if (entreLimites(i + 1, j + 2) and Tablero[i + 1][j + 2].tipo == CABALLO and Tablero[i + 1][j + 2].color != color) return true;
    if (entreLimites(i - 1, j + 2) and Tablero[i - 1][j + 2].tipo == CABALLO and Tablero[i - 1][j + 2].color != color) return true;
    if (entreLimites(i + 1, j - 2) and Tablero[i + 1][j - 2].tipo == CABALLO and Tablero[i + 1][j - 2].color != color) return true;
    if (entreLimites(i - 1, j - 2) and Tablero[i - 1][j - 2].tipo == CABALLO and Tablero[i - 1][j - 2].color != color) return true;
    return false;
}

// Comprueba que no haya ningun peon apuntando a esa casilla
bool apuntaPeon(const VVP& Tablero, int i, int j, int color) {
    // Rey blanco
    if (color == 0 and (
        (entreLimites(i - 1, j + 1) and Tablero[i - 1][j + 1].color == 1 and Tablero[i - 1][j + 1].tipo == PEON) or
        (entreLimites(i - 1, j - 1) and Tablero[i - 1][j - 1].color == 1 and Tablero[i - 1][j - 1].tipo == PEON))) return true;
    // Rey negro
    if (color == 1 and (
        (entreLimites(i + 1, j + 1) and Tablero[i + 1][j + 1].color == 0 and Tablero[i + 1][j + 1].tipo == PEON) or
        (entreLimites(i + 1, j - 1) and Tablero[i + 1][j - 1].color == 0 and Tablero[i + 1][j - 1].tipo == PEON))) return true;
    return false;
}

// Comprueba si la casilla esta amenazada por alguna pieza del otro color
bool atacado (const VVP& Tablero, int i, int j, int color) {
    if (apuntaCaballo(Tablero, i, j, color)) return true;
    if (apuntaAlfil(Tablero, i, j, color)) return true;
    if (apuntaTorre(Tablero, i, j, color)) return true;
    if (apuntaPeon(Tablero, i, j, color)) return true;
    return false;
}

// i = posicion x final, j = posicion y final, x = posicion x inicial, y = posicion y final
// Pre: aux puede valer 1, -1 o 0
bool mov(const VVP& Tablero, int i, int j, int x, int y, int aux1, int aux2) {
    int k = 1;
    while (entreLimites(x + k*aux1, y + k*aux2)) {
        if (x + k*aux1 == i and y + k*aux2 == j) return true;
        if (Tablero[x + k*aux1][y + k*aux2].tipo != VACIO) return false;
        ++k;
    }
    return false;
}

// Comprueba si se puede mover el alfil a esa casilla
bool movAlfil(const VVP& Tablero, int i, int j, int x, int y) {
    if (x < i and y < j and mov(Tablero, i, j, x, y, 1, 1)) return true;
    if (x < i and y > j and mov(Tablero, i, j, x, y, 1, -1)) return true;
    if (x > i and y < j and mov(Tablero, i, j, x, y, -1, 1)) return true;
    if (x > i and y > j and mov(Tablero, i, j, x, y, -1, -1)) return true;
    return false;
}

// Comprueba si se puede mover la torre a esa casilla
bool movTorre(const VVP& Tablero, int i, int j, int x, int y) {
    if (y < j and mov(Tablero, i, j, x, y, 0, 1)) return true;
    if (y > j and mov(Tablero, i, j, x, y, 0, -1)) return true;
    if (x < i and mov(Tablero, i, j, x, y, 1, 0)) return true;
    if (x > i and mov(Tablero, i, j, x, y, -1, 0)) return true;
    return false;
}

// Comprueba si se puede mover el caballo a esa casilla
bool movCaballo(int i, int j, int x, int y) {
    int dx = abs(i - x), dy = abs(j - y);
    if ((dx == 2 && dy == 1) || (dx == 1 && dy == 2)) return true;
    return false;
}

// Comprueba si se puede mover el peon a esa casilla
bool movPeon(const VVP& Tablero, int i, int j, int x, int y, PiezaTipo pieza_prev, int aux_j, int aux_x, int aux_y, bool& captura_al_paso) {
    int color = Tablero[x][y].color;
    int dir = (color == BLANCO ? -1 : 1); // Blancas suben (i disminuye), negras bajan (i aumenta)
    captura_al_paso = false;
    (void)pieza_prev; (void)aux_j; (void)aux_x; (void)aux_y; // legacy params no longer required

    // Movimiento recto (misma columna)
    if (j == y) {
        // Una casilla
        if (i == x + dir && Tablero[i][j].tipo == VACIO) return true;
        // Dos casillas desde la posición inicial
        if (i == x + 2*dir) {
            if ((color == BLANCO && x == 6) || (color == NEGRO && x == 1)) {
                int mid = x + dir;
                if (Tablero[mid][j].tipo == VACIO && Tablero[i][j].tipo == VACIO) return true;
            }
        }
        return false; // mismo archivo pero destino no válido
    }

    // Captura diagonal (una fila hacia delante y una columna a un lado)
    if (abs(j - y) == 1 && i == x + dir) {
        // Captura normal
        if (Tablero[i][j].tipo != VACIO && Tablero[i][j].color != color) return true;
        // Captura al paso: destino vacío, coincide con objetivo en passant válido
        if (Tablero[i][j].tipo == VACIO && g_epRow == i && g_epCol == j) {
            captura_al_paso = true;
            return true;
        }
        return false;
    }

    return false;
}

// Comprueba si se puede mover el rey a esa casilla
bool movRey(const VVP& Tablero, int i, int j, int x, int y) {
    int color = Tablero[x][y].color;
    // Enroque
    if (abs(i - x) == 2 and j - y == 0 and !Tablero[x][y].movido and !atacado(Tablero, 4, j, color)) {
        // Enroque corto
        if (i == 6 and !Tablero[7][j].movido and Tablero[5][j].tipo == VACIO and Tablero[6][j].tipo == VACIO and
            !atacado(Tablero, 5, j, color) and !atacado(Tablero, 6, j, color)) return true;
        // Enroque largo
        if (i == 2 and !Tablero[0][j].movido and Tablero[1][j].tipo == VACIO and
            Tablero[2][j].tipo == VACIO and Tablero[3][j].tipo == VACIO and
            !atacado(Tablero, 3, j, color) and !atacado(Tablero, 2, j, color)) return true;
    }
    // Movimiento normal del rey
    if (abs(i - x) <= 1 && abs(j - y) <= 1 && (i != x || j != y)) return true;
    return false;
}

bool movReina(const VVP& Tablero, int i, int j, int x, int y) {
    return movAlfil(Tablero, i, j, x, y) or movTorre(Tablero, i, j, x, y);
}

void moverPieza(VVP& Tablero, int i, int j, int x, int y, PiezaTipo corona) {
    // Captura al paso: peón se mueve en diagonal a casilla vacía que coincide con objetivo EP
    if (Tablero[x][y].tipo == PEON && x != i && Tablero[i][j].tipo == VACIO && g_epRow == i && g_epCol == j) {
        // El peón capturado está en la fila de origen del peón que captura y en la columna de destino
        Tablero[x][j] = {PiezaTipo::VACIO, COLOR_VACIO, false};
    }
    // Enroque
    else if (Tablero[x][y].tipo == REY && abs(x - i) == 2) {
        // Enroque corto
        if (i == 6) {
            Tablero[5][j] = Tablero[7][j];
            Tablero[7][j] = {PiezaTipo::VACIO, COLOR_VACIO, false};
        }
        // Enroque largo
        else if (i == 2) {
            Tablero[3][j] = Tablero[0][j];
            Tablero[0][j] = {PiezaTipo::VACIO, COLOR_VACIO, false};
        }
    }
    // Movimiento estandar
    Tablero[i][j] = Tablero[x][y];
    Tablero[i][j].movido = true;
    Tablero[x][y] = {PiezaTipo::VACIO, COLOR_VACIO, false};
    // Gestionar objetivo de en passant tras el movimiento
    if (Tablero[i][j].tipo == PEON) {
        if (j == y && abs(i - x) == 2) {
            // Peón acaba de avanzar dos casillas: habilita EP en la casilla intermedia
            g_epRow = x + (i - x) / 2;
            g_epCol = j;
        } else {
            // Cualquier otro movimiento de peón limpia EP
            g_epRow = g_epCol = -1;
        }
    } else {
        // Si movió otra pieza, se invalida EP
        g_epRow = g_epCol = -1;
    }
    // Coronar peones
    // Promoción correcta: cuando el peón alcanza la última fila según su color
    if (Tablero[i][j].tipo == PEON && (i == 0 || i == 7)) Tablero[i][j].tipo = corona;
}

// Recrea el movimiento y comprueba que el rey no este en jaque
bool jaque(VVP Tablero, int i, int j, int x, int y, int turno) {
    // Guardar y restaurar objetivo de en passant para no contaminar estado global en simulaciones
    int oldEpRow = g_epRow, oldEpCol = g_epCol;
    moverPieza(Tablero, i, j, x, y, REINA);
    bool enJaque = false;
    for (int k = 0; k < BOARD_SIZE && !enJaque; ++k) {
        for (int l = 0; l < BOARD_SIZE && !enJaque; ++l) {
            if (Tablero[k][l].tipo == REY and Tablero[k][l].color == turno) {
                enJaque = atacado(Tablero, k, l, turno);
            }
        }
    }
    g_epRow = oldEpRow; g_epCol = oldEpCol;
    return enJaque;
}

// Muestra las posiciones válidas resaltadas en el tablero usando ANSI (verde)
void mostrarPosicionesValidas(const VVB& PV, const VVP& Tablero) {
    dibujarTablero(Tablero, &PV);
}

VVB calcularPosicionesValidas(const VVP& Tablero, int x, int y, PiezaTipo pieza_prev, int aux_j, int aux_x, int aux_y, int turno) {
    VVB PV(BOARD_SIZE, VB(BOARD_SIZE, false));
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            bool valida = false;
            bool captura_al_paso = false;
            switch (Tablero[x][y].tipo) {
                case PEON:
                    if (movPeon(Tablero, i, j, x, y, pieza_prev, aux_j, aux_x, aux_y, captura_al_paso)) valida = true;
                    break;
                case CABALLO:
                    if (movCaballo(i, j, x, y)) valida = true;
                    break;
                case ALFIL:
                    if (movAlfil(Tablero, i, j, x, y)) valida = true;
                    break;
                case TORRE:
                    if (movTorre(Tablero, i, j, x, y)) valida = true;
                    break;
                case REINA:
                    if (movReina(Tablero, i, j, x, y)) valida = true;
                    break;
                case REY:
                    if (movRey(Tablero, i, j, x, y)) valida = true;
                    break;
                default:
                    break;
            }
            if (Tablero[x][y].color == Tablero[i][j].color) valida = false;
            if (valida and !jaque(Tablero, i, j, x, y, turno)) PV[i][j] = true;
        }
    }
    return PV;
}

bool existeMovimientoValido(const VVP& Tablero, PiezaTipo pieza_prev, int aux_j, int aux_x, int aux_y, int turno) {
    // Encuentra una pieza aliada
    for (int x = 0; x < BOARD_SIZE; ++x) {
        for (int y = 0; y < BOARD_SIZE; ++y) {
            if (Tablero[x][y].color == turno) {
                VVB PV = calcularPosicionesValidas(Tablero, x, y, pieza_prev, aux_j, aux_x, aux_y, turno);
                for (int i = 0; i < BOARD_SIZE; ++i) {
                    for (int j = 0; j < BOARD_SIZE; ++j) {
                        if (PV[i][j]) return true;
                    }
                }
            }
            
        }
    }
    return false;
}

bool mate(const VVP& Tablero, PiezaTipo pieza_prev, int aux_j, int aux_x, int aux_y, int turno) {
    // Encuentra el rey
    for (int i = 0; i < BOARD_SIZE; ++i) {
        for (int j = 0; j < BOARD_SIZE; ++j) {
            if (Tablero[i][j].tipo == REY and Tablero[i][j].color == turno) {
                // Comprueba si el rey esta en jaque
                if (atacado(Tablero, i, j, turno)) {
                    // Comprueba si existe un movimiento valido para el jugador actual
                    if (!existeMovimientoValido(Tablero, pieza_prev, aux_j, aux_x, aux_y, turno)) {
                        return true;
                    }
                }
                return false;
            }
        }
    }
    return false;
}

// Simulación básica de lectura de clics de ratón en terminal (xterm)
// Devuelve las coordenadas de la celda clickeada
// Obtiene la celda seleccionada por clic de ratón en terminal
// Conserva la API anterior, pero no se usa en el nuevo flujo
void obtenerPosicionRaton(int& i, int& j, int& x, int& y) {
    obtenerOrigenRaton(i, j);
    x = i; y = j;
    obtenerDestinoTrasOrigen(i, j, x, y);
}

int main() {
    VVP Tablero = crearTablero();
    imprimirTablero(Tablero);

    int turno = 0;
    int auxpos_yi = 0, auxpos_y = 0, auxpos_xi = 0;
    PiezaTipo pieza_prev = VACIO;

    while (true) {
        int i0, j0;
        // Bucle de selección del origen: solo sale cuando se selecciona una pieza propia válida
        while (true) {
            obtenerOrigenRaton(i0, j0);
            if (!entreLimites(i0, j0)) {
                deshabilitarTrackingRaton();
                imprimirMensaje("Haz clic dentro del tablero", MSG_SHORT);
                continue;
            }
            if (Tablero[i0][j0].tipo == VACIO) {
                deshabilitarTrackingRaton();
                imprimirMensaje("Casilla vacía. Elige una pieza.", MSG_SHORT);
                continue;
            }
            if (Tablero[i0][j0].color != turno) {
                deshabilitarTrackingRaton();
                imprimirMensaje("Esa pieza no es tuya. Selecciona una pieza de tu color.", MSG_SHORT);
                continue;
            }
            break; // selección correcta
        }

        // Dentro del bucle: mostrar PV y recoger destino hasta que sea válido.
        int i_dest = -1, j_dest = -1;
        bool showHighlights = true;
        while (i_dest == -1 or j_dest == -1) {
            VVB PV = calcularPosicionesValidas(Tablero, i0, j0, pieza_prev, auxpos_yi, auxpos_xi, auxpos_y, turno);
            if (showHighlights) mostrarPosicionesValidas(PV, Tablero);
            int i1, j1;
            obtenerDestinoTrasOrigen(i0, j0, i1, j1); // release del drag o segundo clic
            // Si clicas otra pieza tuya, re-selecciona origen y repite
            if (entreLimites(i1, j1) && Tablero[i1][j1].color == turno) {
                i0 = i1; j0 = j1;
                imprimirMensaje(string("Pieza re-seleccionada: ") + toAlg(i0, j0), MSG_SHORT);
                // Limpiar resaltado anterior
                imprimirTablero(Tablero);
                // Rehabilita tracking para volver a pedir destino
                habilitarTrackingRaton();
                showHighlights = true; // volver a mostrar PV para la nueva pieza
                continue;
            }
            // Si el destino no es válido, permite reintentar destino con mismo origen
            if (!entreLimites(i1, j1) || !PV[i1][j1]) {
                imprimirMensaje("Movimiento no válido. Intenta otra casilla.", MSG_MEDIUM);
                // Limpiar resaltado de PV
                imprimirTablero(Tablero);
                habilitarTrackingRaton();
                showHighlights = false; // mantener limpio hasta nuevo intento
                continue;
            }
            i_dest = i1; j_dest = j1;
        }

        // Coronar peón si aplica
        PiezaTipo corona = REINA;
        if (Tablero[i0][j0].tipo == PEON and (i_dest == 0 or i_dest == 7)) {
            char c = coronarPeones();
            switch (c) {
                case 'Q': corona = REINA; break;
                case 'R': corona = TORRE; break;
                case 'B': corona = ALFIL; break;
                case 'N': corona = CABALLO; break;
                default: corona = REINA; break;
            }
        }

        // Guardar jugada anterior para captura al paso
        pieza_prev = Tablero[i0][j0].tipo;
        auxpos_yi = j0;
        auxpos_xi = i0;
        auxpos_y = j_dest;

        moverPieza(Tablero, i_dest, j_dest, i0, j0, corona);
        imprimirMensaje(string("Movido ") + toAlg(i0, j0) + " → " + toAlg(i_dest, j_dest), MSG_MEDIUM);
        imprimirTablero(Tablero);

        // Comprobar mate
        if (mate(Tablero, pieza_prev, auxpos_yi, auxpos_xi, auxpos_y, 1 - turno)) {
            imprimirMensaje(turno == 0 ? "Jaque mate: ganan Blancas" : "Jaque mate: ganan Negras", MSG_LONG, false);
            break;
        }
        // Aviso de jaque simple (si el rival queda en jaque)
        // Busca rey rival y comprueba amenaza
        for (int r = 0; r < BOARD_SIZE; ++r) {
            for (int c = 0; c < BOARD_SIZE; ++c) {
                if (Tablero[r][c].tipo == REY && Tablero[r][c].color == 1 - turno) {
                    if (atacado(Tablero, r, c, 1 - turno)) {
                        imprimirMensaje("Jaque", MSG_MEDIUM);
                    }
                }
            }
        }
    turno = 1 - turno;
    }
}