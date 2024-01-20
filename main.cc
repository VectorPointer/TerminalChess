#include <iostream>
#include <vector>
using namespace std;

// Definir codigos de escape ANSI para colores
#define RESET   "\033[0m"
#define BLACK   "\033[40m"  // Fondo negro
#define WHITE   "\033[47m"  // Fondo blanco
#define WHITE_TEXT "\033[1;37m" // Texto blanco para los elementos auxiliares
#define BLACK_PIECE "\033[1;31m"  // Rojo brillante para las piezas negras
#define WHITE_PIECE "\033[1;34m"  // Azul brillante para las piezas blancas

struct Pieza {
    char nombre;
    int color; // Blanco 0, negro 1, vacio -1
    bool movido; // Se utiliza para el enroque
};

typedef vector<Pieza> VP;
typedef vector<VP> VVP;

int abs(int a) {
    if (a < 0) return -a;
    return a;
}

VVP crearTablero() {
    VVP Tablero(8, VP(8, {' ', -1, false}));

    // Colores
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 8; ++j) Tablero[i][j].color = 1;
    }
    
    for (int i = 6; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) Tablero[i][j].color = 0;
    }
    
    // P = Pawn
    for (int i = 0; i < 8; ++i) {
        Tablero[1][i].nombre = 'P';
    }

    for (int i = 0; i < 8; ++i) {
        Tablero[6][i].nombre = 'P';
    }

    // Piezas: R = Rook, N = Knight, B = Bishop, Q = Queen, K = King
    Tablero[0][0].nombre = 'R';
    Tablero[0][1].nombre = 'N';
    Tablero[0][2].nombre = 'B';
    Tablero[0][3].nombre = 'Q';
    Tablero[0][4].nombre = 'K';
    Tablero[0][5].nombre = 'B';
    Tablero[0][6].nombre = 'N';
    Tablero[0][7].nombre = 'R';

    Tablero[7][0].nombre = 'R';
    Tablero[7][1].nombre = 'N';
    Tablero[7][2].nombre = 'B';
    Tablero[7][3].nombre = 'Q';
    Tablero[7][4].nombre = 'K';
    Tablero[7][5].nombre = 'B';
    Tablero[7][6].nombre = 'N';
    Tablero[7][7].nombre = 'R';

    return Tablero;
}

void imprimirTablero(const VVP& Tablero) {
    cout << "  ";
    for (int i = 0; i <= 16; ++i) cout << WHITE_TEXT << '-' << RESET;
    cout << endl;

    for (int i = 0; i < 8; ++i) {
        cout << WHITE_TEXT << 8 - i << RESET << ' ';
        for (int j = 0; j < 8; ++j) {
            cout << WHITE_TEXT << '|' << RESET;
            if ((i + j) % 2 == 0) cout << WHITE;
            else cout << BLACK;

            if (Tablero[i][j].color == 1) cout << BLACK_PIECE;
            else if (Tablero[i][j].color == 0) cout << WHITE_PIECE;
            
            cout << Tablero[i][j].nombre << RESET;
        }
        cout << WHITE_TEXT << '|' << RESET << endl << "  ";
        for (int j = 0; j <= 16; ++j) {
            cout << WHITE_TEXT << '-' << RESET;
        }
        cout << endl;
    }
    cout << "  ";
    for (int i = 0; i < 8; ++i) cout << ' ' << WHITE_TEXT << char('A' + i) << RESET;
    cout << endl;
}

bool movCaballo(int pos_x, int pos_y, int pos_xi, int pos_yi) {
    if (
        (pos_y != pos_yi + 2 or pos_x != pos_xi + 1) and
        (pos_y != pos_yi + 2 or pos_x != pos_xi - 1) and
        (pos_y != pos_yi - 2 or pos_x != pos_xi + 1) and
        (pos_y != pos_yi - 2 or pos_x != pos_xi - 1) and
        (pos_y != pos_yi + 1 or pos_x != pos_xi + 2) and
        (pos_y != pos_yi - 1 or pos_x != pos_xi + 2) and
        (pos_y != pos_yi + 1 or pos_x != pos_xi - 2) and
        (pos_y != pos_yi - 1 or pos_x != pos_xi - 2)) return false;
    return true;
}

bool movAlfil(const VVP& Tablero, int pos_x, int pos_y, int pos_xi, int pos_yi) {
    int i = 1;
    // Comprueba en que diagonal tiene que moverse
    if (pos_xi < pos_x and pos_yi < pos_y) {
        // Comprueba que no hay obstaculos (excepto en la casilla final)
        while (i < 8 and pos_yi + i < 8 and pos_xi + i < 8 and 
        (Tablero[pos_yi + i][pos_xi + i].nombre == ' ' or (pos_yi + i == pos_y and pos_xi + i == pos_x))) {
            if (pos_yi + i == pos_y and pos_xi + i == pos_x) return true;
            ++i;
        }
    }
    else if (pos_xi > pos_x and pos_yi < pos_y) {
        while (i < 8 and pos_yi + i < 8 and pos_xi - i >= 0 and 
        (Tablero[pos_yi + i][pos_xi - i].nombre == ' ' or (pos_yi + i == pos_y and pos_xi - i == pos_x))) {
            if (pos_yi + i == pos_y and pos_xi - i == pos_x) return true;
            ++i;
        }
    }
    else if (pos_xi < pos_x and pos_yi > pos_y) {
        while (i < 8 and pos_yi - i >= 0 and pos_xi + i < 8 and 
        (Tablero[pos_yi - i][pos_xi + i].nombre == ' ' or (pos_yi - i == pos_y and pos_xi + i == pos_x))) {
            if (pos_yi - i == pos_y and pos_xi + i == pos_x) return true;
            ++i;
        }
    }
    else if (pos_xi > pos_x and pos_yi > pos_y){
        while (i < 8 and pos_yi - i >= 0 and pos_xi - i >= 0 and 
        (Tablero[pos_yi - i][pos_xi - i].nombre == ' ' or (pos_yi - i == pos_y and pos_xi - i == pos_x))) {
            if (pos_yi - i == pos_y and pos_xi - i == pos_x) return true;
            ++i;
        }
    }
    return false;
}

bool movTorre(const VVP& Tablero, int pos_x, int pos_y, int pos_xi, int pos_yi) {
    int i = 1;
    // Comprueba en que direccion tiene que moverse
    if (pos_yi < pos_y) {
        // Comprueba que no hay obstaculos (excepto en la casilla final)
        while (i < 8 and pos_yi + i < 8 and 
        (Tablero[pos_yi + i][pos_xi].nombre == ' ' or (pos_yi + i == pos_y and pos_xi == pos_x))) {
            if (pos_yi + i == pos_y and pos_xi == pos_x) return true;
            ++i;
        }
    }

    else if (pos_yi > pos_y) {
        while (i < 8 and pos_yi - i >= 0 and 
        (Tablero[pos_yi - i][pos_xi].nombre == ' ' or (pos_yi - i == pos_y and pos_xi == pos_x))) {
            if (pos_yi - i == pos_y and pos_xi == pos_x) return true;
            ++i;
        }
    }

    else if (pos_xi < pos_x) {
        while (i < 8 and pos_xi + i < 8 and 
        (Tablero[pos_yi][pos_xi + i].nombre == ' ' or (pos_yi == pos_y and pos_xi + i == pos_x))) {
            if (pos_yi == pos_y and pos_xi + i == pos_x) return true;
            ++i;
        }
    }

    else if (pos_xi > pos_x){
        while (i < 8 and pos_xi - i >= 0 and 
        (Tablero[pos_yi][pos_xi - i].nombre == ' ' or (pos_yi == pos_y and pos_xi - i == pos_x))) {
            if (pos_yi == pos_y and pos_xi - i == pos_x) return true;
            ++i;
        }
    }
    return false;
}

bool movRey(const VVP& Tablero, int pos_x, int pos_y, int pos_xi, int pos_yi) {
    if (abs(pos_x - pos_xi) == 2 and not Tablero[pos_yi][pos_xi].movido) {
        // Enroque corto
        if (pos_x == 6 and not Tablero[pos_y][7].movido and 
        Tablero[pos_y][5].nombre == ' ' and Tablero[pos_y][6].nombre == ' ') return true;
        // Enroque largo
        if (pos_x == 2 and not Tablero[pos_y][0].movido and Tablero[pos_y][1].nombre == ' ' and 
        Tablero[pos_y][2].nombre == ' ' and Tablero[pos_y][3].nombre == ' ') return true;
    }
    if (pos_y == pos_yi and pos_x == pos_xi + 1) return true;
    if (pos_y == pos_yi and pos_x == pos_xi - 1) return true;
    if (pos_y == pos_yi + 1 and pos_x == pos_xi) return true;
    if (pos_y == pos_yi - 1 and pos_x == pos_xi) return true;
    if (pos_y == pos_yi + 1 and pos_x == pos_xi + 1) return true;
    if (pos_y == pos_yi + 1 and pos_x == pos_xi - 1) return true;
    if (pos_y == pos_yi - 1 and pos_x == pos_xi + 1) return true;
    if (pos_y == pos_yi - 1 and pos_x == pos_xi - 1) return true;
        
    return false;
}

void moverPieza(VVP& Tablero, int pos_x, int pos_y, int pos_xi, int pos_yi, 
char pieza_prev, int auxpos_yi, int auxpos_y, char corona) {
    // Captura al paso
    if (abs(pos_yi - pos_y) == 1 and abs(pos_x - pos_xi) == 1 and pieza_prev == 'P' and abs(auxpos_y - auxpos_yi) == 2) {
        Tablero[auxpos_y][pos_x] = {' ', -1, false};
    }

    // Enroque corto
    else if (pos_x == 6 and not Tablero[pos_y][7].movido and Tablero[pos_y][5].nombre == ' ' and 
    Tablero[pos_y][6].nombre == ' ') {
        Tablero[pos_y][5] = Tablero[pos_y][7];
        Tablero[pos_y][7] = {' ', -1, false};
    }

    // Enroque largo
    else if (pos_x == 2 and not Tablero[pos_y][0].movido and Tablero[pos_y][1].nombre == ' ' and 
    Tablero[pos_y][2].nombre == ' ' and Tablero[pos_y][3].nombre == ' ') {
        Tablero[pos_y][3] = Tablero[pos_y][0];
        Tablero[pos_y][0] = {' ', -1, false};
    }

    // Movimiento estandar
    Tablero[pos_y][pos_x] = Tablero[pos_yi][pos_xi];
    Tablero[pos_y][pos_x].movido = true;
    Tablero[pos_yi][pos_xi] = {' ', -1, false};

    // Coronar peones
    if (Tablero[pos_y][pos_x].nombre == 'P' and (pos_y == 0 or pos_y == 7)) {
        Tablero[pos_y][pos_x].nombre = corona;
    }
}

bool apuntaCaballo(const VVP& Tablero, int i, int j, int color) {
    if (i + 2 < 8 and j + 1 < 8 and Tablero[i + 2][j + 1].nombre == 'N' and Tablero[i + 2][j + 1].color != color) return true;
    if (i + 2 < 8 and j - 1 >= 0 and Tablero[i + 2][j - 1].nombre == 'N' and Tablero[i + 2][j - 1].color != color) return true;
    if (i - 2 >= 0 and j + 1 < 8 and Tablero[i - 2][j + 1].nombre == 'N' and Tablero[i - 2][j + 1].color != color) return true;
    if (i - 2 >= 0 and j - 1 >= 0 and Tablero[i - 2][j - 1].nombre == 'N' and Tablero[i - 2][j - 1].color != color) return true;
    if (i + 1 < 8 and j + 2 < 8 and Tablero[i + 1][j + 2].nombre == 'N' and Tablero[i + 1][j + 2].color != color) return true;
    if (i - 1 >= 0 and j + 2 < 8 and Tablero[i - 1][j + 2].nombre == 'N' and Tablero[i - 1][j + 2].color != color) return true;
    if (i + 1 < 8 and j - 2 >= 0 and Tablero[i + 1][j - 2].nombre == 'N' and Tablero[i + 1][j - 2].color != color) return true;
    if (i - 1 >= 0 and j - 2 >= 0 and Tablero[i - 1][j - 2].nombre == 'N' and Tablero[i - 1][j - 2].color != color) return true;
    return false;
}

// Comprueba que no haya ningun alfil apuntando a esa casilla
bool apuntaAlfil(const VVP& Tablero, int i, int j, int color) {
    int k = 1;
    // Comprueba que este dentro de los limites, no haya obstaculos o esten atacando en diagonal
    while (k < 8 and i + k < 8 and j + k < 8 and Tablero[i + k][j + k].color != color and
    (Tablero[i + k][j + k].nombre == ' ' or Tablero[i + k][j + k].nombre == 'B' or Tablero[i + k][j + k].nombre == 'Q')) {
        if (Tablero[i + k][j + k].nombre == 'B' or Tablero[i + k][j + k].nombre == 'Q') return true;
        ++k;
    }

    k = 1;
    while (k < 8 and i + k < 8 and j - k >= 0 and Tablero[i + k][j - k].color != color and
    (Tablero[i + k][j - k].nombre == ' ' or Tablero[i + k][j - k].nombre == 'B' or Tablero[i + k][j - k].nombre == 'Q')) {
        if (Tablero[i + k][j - k].nombre == 'B' or Tablero[i + k][j - k].nombre == 'Q') return true;
        ++k;
    }
    
    k = 1; 
    while (k < 8 and i - k >= 0 and j + k < 8 and Tablero[i - k][j + k].color != color and
    (Tablero[i - k][j + k].nombre == ' ' or Tablero[i - k][j + k].nombre == 'B' or Tablero[i - k][j + k].nombre == 'Q')) {
        if (Tablero[i - k][j + k].nombre == 'B' or Tablero[i - k][j + k].nombre == 'Q') return true;
        ++k;
    }

    k = 1; 
    while (k < 8 and i - k >= 0 and j - k >= 0 and Tablero[i - k][j - k].color != color and
    (Tablero[i - k][j - k].nombre == ' ' or Tablero[i - k][j - k].nombre == 'B' or Tablero[i - k][j - k].nombre == 'Q')) {
        if (Tablero[i - k][j - k].nombre == 'B' or Tablero[i - k][j - k].nombre == 'Q') return true;
        ++k;
    }
    return false;
}

// Comprueba que no haya ninguna torre apuntado a esa casilla
bool apuntaTorre(const VVP& Tablero, int i, int j, int color) {
    int k = 1;
    // Comprueba que este dentro de los limites, no haya obstaculos o esten atacando en horizontal o vertical
    while (k < 8 and i + k < 8 and Tablero[i + k][j].color != color and
    (Tablero[i + k][j].nombre == ' ' or Tablero[i + k][j].nombre == 'R' or Tablero[i + k][j].nombre == 'Q')) {
        if (Tablero[i + k][j].nombre == 'R' or Tablero[i + k][j].nombre == 'Q') return true;
        ++k;
    }

    k = 1;
    while (k < 8 and i - k >= 0 and Tablero[i - k][j].color != color and
    (Tablero[i - k][j].nombre == ' ' or Tablero[i - k][j].nombre == 'R' or Tablero[i - k][j].nombre == 'Q')) {
        if (Tablero[i - k][j].nombre == 'R' or Tablero[i - k][j].nombre == 'Q') return true;
        ++k;
    }

    k = 1;
    while (k < 8 and j + k < 8 and Tablero[i][j + k].color != color and
    (Tablero[i][j + k].nombre == ' ' or Tablero[i][j + k].nombre == 'R' or Tablero[i][j + k].nombre == 'Q')) {
        if (Tablero[i][j + k].nombre == 'R' or Tablero[i][j + k].nombre == 'Q') return true;
        ++k;
    }

    k = 1;
    while (k < 8 and j - k >= 0 and Tablero[i][j - k].color != color and
    (Tablero[i][j - k].nombre == ' ' or Tablero[i][j - k].nombre == 'R' or Tablero[i][j - k].nombre == 'Q')) {
        if (Tablero[i][j - k].nombre == 'R' or Tablero[i][j - k].nombre == 'Q') return true;
        ++k;
    }
    return false;
}

bool apuntaPeon(const VVP& Tablero, int i, int j, int color) {
    // Rey blanco
    if (i - 1 >= 0 and color == 0 and (
    (j + 1 < 8 and Tablero[i - 1][j + 1].color == 1 and Tablero[i - 1][j + 1].nombre == 'P') or 
    (j - 1 >= 0 and Tablero[i - 1][j - 1].color == 1 and Tablero[i - 1][j - 1].nombre == 'P'))) return true;


    // Rey negro
    if (i + 1 < 8 and color == 1 and (
    (j + 1 < 8 and Tablero[i + 1][j + 1].color == 0 and Tablero[i + 1][j + 1].nombre == 'P') or 
    (j - 1 >= 0 and Tablero[i + 1][j - 1].color == 0 and Tablero[i + 1][j - 1].nombre == 'P'))) return true;
    
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

// Recrea el movimiento y comprueba que el rey no este en jaque
bool jaque(VVP Tablero, int pos_x, int pos_y, int pos_xi, int pos_yi, int turno, char corona) {
    moverPieza(Tablero, pos_x, pos_y, pos_xi, pos_yi, ' ', -1, -1, corona);
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            if (Tablero[i][j].nombre == 'K' and Tablero[i][j].color == turno) {
                int color = Tablero[i][j].color;
                if (atacado(Tablero, i, j, color)) return true;
                return false;
            }
        }
    }
    return false;
}

bool posicionValida(const VVP& Tablero, int pos_x, int pos_y, int pos_xi, int pos_yi, 
char pieza_prev, int auxpos_yi, int auxpos_y, bool turno, char corona) {
    // Fuera de los limites
    if (pos_y < 0 or pos_y >= 8 or pos_x < 0 or pos_x >= 8) return false;

    // No comer mismo color
    if (Tablero[pos_yi][pos_xi].color == Tablero[pos_y][pos_x].color) return false;

    // Mover fuera de turno
    if (Tablero[pos_yi][pos_xi].color != turno) return false;

    // Peon
    if (Tablero[pos_yi][pos_xi].nombre == 'P') {
        // Avanzar
        if (pos_yi <= pos_y and Tablero[pos_yi][pos_xi].color == 0) return false;   // Retroceder
        if (pos_yi >= pos_y and Tablero[pos_yi][pos_xi].color == 1) return false;   // Retroceder

        if (abs(pos_yi - pos_y) > 2) return false;   // Avanzar mas de 2 casillas
        if (pos_xi == pos_x) {
            // Avance de 2 casillas
            for (int i = 0; i < abs(pos_y - pos_yi); ++i) {
                if (Tablero[pos_y - i][pos_x].nombre != ' ') return false;
            }
        }
        
        // Capturar
        if (abs(pos_y - pos_yi) == 1 and abs(pos_x - pos_xi) == 1) {
            // Captura al paso
            if (abs(pos_yi - pos_y) == 1 and abs(pos_x - pos_xi) == 1
            and pieza_prev == 'P' and abs(auxpos_y - auxpos_yi) == 2) return true;

            if (Tablero[pos_y][pos_x].nombre == ' ') return false;
        }
    }

    // Caballo
    else if (Tablero[pos_yi][pos_xi].nombre == 'N') {
        if (not movCaballo(pos_x, pos_y, pos_xi, pos_yi)) return false;
    }

    // Alfil
    else if (Tablero[pos_yi][pos_xi].nombre == 'B') {
        // Mismo color de casilla
        if ((pos_yi + pos_xi)%2 != (pos_y + pos_x)%2) return false;
        
        if(not movAlfil(Tablero, pos_x, pos_y, pos_xi, pos_yi)) return false;
    }

    // Torre
    else if (Tablero[pos_yi][pos_xi].nombre == 'R') {
        // Se mantiene la fila o la columna
        if (pos_yi != pos_y and pos_xi != pos_x) return false;

        if (not movTorre(Tablero, pos_x, pos_y, pos_xi, pos_yi)) return false;
    }

    // Reina
    else if (Tablero[pos_yi][pos_xi].nombre == 'Q') {
        if (not movAlfil(Tablero, pos_x, pos_y, pos_xi, pos_yi) and 
        not movTorre(Tablero, pos_x, pos_y, pos_xi, pos_yi)) return false;
    }

    // Rey
    else if (Tablero[pos_yi][pos_xi].nombre == 'K' and not movRey(Tablero, pos_x, pos_y, pos_xi, pos_yi)) return false;

    // No ignorar jaque
    if (jaque(Tablero, pos_x, pos_y, pos_xi, pos_yi, turno, corona)) return false;

    return true;
}

bool existeMovimientoValido(const VVP& Tablero, int turno) {
    for (int pos_y = 0; pos_y < 8; ++pos_y) {
        for (int pos_x = 0; pos_x < 8; ++pos_x) {
            if (Tablero[pos_y][pos_x].color == turno) {
                for (int pos_yi = 0; pos_yi < 8; ++pos_yi) {
                    for (int pos_xi = 0; pos_xi < 8; ++pos_xi) {
                        char pieza_prev = Tablero[pos_yi][pos_xi].nombre;
                        int auxpos_yi = pos_yi;
                        int auxpos_y = pos_y;

                        if (posicionValida(Tablero, pos_x, pos_y, pos_xi, pos_yi, pieza_prev, auxpos_yi, auxpos_y, turno, ' ')) return true;
                    }
                }
            }
        }
    }
    return false;
}

bool mate(const VVP& Tablero, int turno) {
    // Encuentra el rey
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            if (Tablero[i][j].nombre == 'K' and Tablero[i][j].color == turno) {
                // Comprueba si el rey esta en jaque
                if (atacado(Tablero, i, j, turno)) {
                    // Comprueba si existe un movimiento valido para el jugador actual
                    if (not existeMovimientoValido(Tablero, turno)) {
                        return true;
                    }
                }
                return false;
            }
        }
    }
    return false;
}

char coronarPeones() {
    // Coronar peones
    char corona;
    cout << "Introduce la pieza a la que quieres coronar: (Q, R, B, N): ";
    cin >> corona;
    while (corona != 'Q' and corona != 'R' and corona != 'B' and corona != 'N') {
        cout << "No puedes coronar el peon a esa pieza" << endl;
        cin >> corona;
    }
    return corona;
}

int main() {
    VVP Tablero = crearTablero();
    imprimirTablero(Tablero);
    
    char posc_xi;
    char posc_yi;
    char posc_x;
    char posc_y;

    int turno = 0;  // 0 = blancas, 1 = negras

    // Guarda la jugada anterior (para la captura al paso)
    int auxpos_yi, auxpos_y;
    auxpos_yi = auxpos_y = 0;
    char pieza_prev = ' ';
    while (cin >> posc_xi >> posc_yi >> posc_x >> posc_y) {
        int pos_xi = int(posc_xi - 'a');
        int pos_x = int(posc_x - 'a');
        int pos_yi = 8 - int(posc_yi - '0');
        int pos_y = 8 - int(posc_y - '0');

        // Introducir pieza a la que se quiere coronar
        char corona = ' ';
        if (Tablero[pos_yi][pos_xi].nombre == 'P' and (pos_y == 0 or pos_y == 7)) corona = coronarPeones();

        if (posicionValida(Tablero, pos_x, pos_y, pos_xi, pos_yi, pieza_prev, auxpos_yi, auxpos_y, turno, corona)) {
            moverPieza(Tablero, pos_x, pos_y, pos_xi, pos_yi, pieza_prev, auxpos_yi, auxpos_y, corona);
            imprimirTablero(Tablero);

            // Cambia el turno
            turno = 1 - turno;

            // Actualiza la jugada anterior (para la captura al paso)
            pieza_prev = Tablero[pos_y][pos_x].nombre;
            auxpos_yi = pos_yi;
            auxpos_y = pos_y;

            // Comprueba si hay mate
            if (mate(Tablero, turno)) {
                cout << "¡Jaque mate! ";
                if (turno == 0) cout << "Las negras ganan." << endl;
                else cout << "Las blancas ganan." << endl;
                break;
            }
        }
        else cout << "Posicion invalida" << endl;
    }
}