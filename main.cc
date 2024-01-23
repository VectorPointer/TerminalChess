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

bool entreLimites(int i, int j) {
    if (i < 8 and j < 8 and i >= 0 and j >= 0) return true;
    return false;
}

VVP crearTablero() {
    VVP Tablero(8, VP(8, {' ', -1, false}));

    // Colores
    for (int i = 0; i < 8; ++i) for (int j = 0; j < 2; ++j) Tablero[i][j].color = 1;
    for (int i = 0; i < 8; ++i) for (int j = 6; j < 8; ++j) Tablero[i][j].color = 0;
    
    // P = Pawn
    for (int i = 0; i < 8; ++i) Tablero[i][1].nombre = 'P';
    for (int i = 0; i < 8; ++i) Tablero[i][6].nombre = 'P';

    // Piezas: R = Rook, N = Knight, B = Bishop, Q = Queen, K = King
    Tablero[0][0].nombre = 'R';
    Tablero[1][0].nombre = 'N';
    Tablero[2][0].nombre = 'B';
    Tablero[3][0].nombre = 'Q';
    Tablero[4][0].nombre = 'K';
    Tablero[5][0].nombre = 'B';
    Tablero[6][0].nombre = 'N';
    Tablero[7][0].nombre = 'R';

    Tablero[0][7].nombre = 'R';
    Tablero[1][7].nombre = 'N';
    Tablero[2][7].nombre = 'B';
    Tablero[3][7].nombre = 'Q';
    Tablero[4][7].nombre = 'K';
    Tablero[5][7].nombre = 'B';
    Tablero[6][7].nombre = 'N';
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

            if (Tablero[j][i].color == 1) cout << BLACK_PIECE;
            else if (Tablero[j][i].color == 0) cout << WHITE_PIECE;
            
            cout << Tablero[j][i].nombre << RESET;
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

// Comprueba si hay captura al paso
bool enPassant(int i, int j, int x, int y, char pieza_prev, int aux_j, int aux_x, int aux_y) {
    // Peon
    if (pieza_prev != 'P') return false;
    // Ultimo movimiento de 2 de distancia
    if (abs(aux_y - aux_j) != 2) return false;
    // Destino = ultimo movimiento
    if (i != aux_x) return false;
    // Come en diagonal
    if (abs(x - i) != 1) return false;
    if (abs(y - j) != 1) return false;
    return true;
}

// Busca en vertical, horizontal y diagonales dependiendo de los valores de aux
// Pre: aux puede valer 1, -1 o 0
bool apunta(const VVP& Tablero, int i, int j, int color, int aux1, int aux2, char pieza) {
    int k = 1;
    while (entreLimites(i + k*aux1, j + k*aux2) and Tablero[i + k*aux1][j + k*aux2].color != color) {
        if (Tablero[i + k*aux1][j + k*aux2].nombre == pieza) return true;
        // Comprueba al mismo tiempo que no haya una reina
        if (Tablero[i + k*aux1][j + k*aux2].nombre == 'Q') return true;
        if (Tablero[i + k*aux1][j + k*aux2].nombre != ' ') break;
        ++k;
    }
    return false;
}

// Comprueba que no haya ningun alfil apuntando a esa casilla
bool apuntaAlfil(const VVP& Tablero, int i, int j, int color) {
    if (apunta(Tablero, i, j, color, 1, 1, 'B')) return true;
    if (apunta(Tablero, i, j, color, 1, -1, 'B')) return true;
    if (apunta(Tablero, i, j, color, -1, 1, 'B')) return true;
    if (apunta(Tablero, i, j, color, -1, -1, 'B')) return true;
    return false;
}

// Comprueba que no haya ninguna torre apuntando a esa casilla
bool apuntaTorre(const VVP& Tablero, int i, int j, int color) {
    if (apunta(Tablero, i, j, color, 1, 0, 'T')) return true;
    if (apunta(Tablero, i, j, color, -1, 0, 'T')) return true;
    if (apunta(Tablero, i, j, color, 0, 1, 'T')) return true;
    if (apunta(Tablero, i, j, color, 0, -1, 'T')) return true;
    return false;
}

// Comprueba que no haya ningun caballo apuntando a esa casilla
bool apuntaCaballo(const VVP& Tablero, int i, int j, int color) {
    if (entreLimites(i + 2, j + 1) and Tablero[i + 2][j + 1].nombre == 'N' and Tablero[i + 2][j + 1].color != color) return true;
    if (entreLimites(i + 2, j - 1) and Tablero[i + 2][j - 1].nombre == 'N' and Tablero[i + 2][j - 1].color != color) return true;
    if (entreLimites(i - 2, j + 1) and Tablero[i - 2][j + 1].nombre == 'N' and Tablero[i - 2][j + 1].color != color) return true;
    if (entreLimites(i - 2, j - 1) and Tablero[i - 2][j - 1].nombre == 'N' and Tablero[i - 2][j - 1].color != color) return true;
    if (entreLimites(i + 1, j + 2) and Tablero[i + 1][j + 2].nombre == 'N' and Tablero[i + 1][j + 2].color != color) return true;
    if (entreLimites(i - 1, j + 2) and Tablero[i - 1][j + 2].nombre == 'N' and Tablero[i - 1][j + 2].color != color) return true;
    if (entreLimites(i + 1, j - 2) and Tablero[i + 1][j - 2].nombre == 'N' and Tablero[i + 1][j - 2].color != color) return true;
    if (entreLimites(i - 1, j - 2) and Tablero[i - 1][j - 2].nombre == 'N' and Tablero[i - 1][j - 2].color != color) return true;
    return false;
}

// Comprueba que no haya ningun peon apuntando a esa casilla
bool apuntaPeon(const VVP& Tablero, int i, int j, int color) {
    // Rey blanco
    if (color == 0 and (
    (entreLimites(i - 1, j + 1) and Tablero[i - 1][j + 1].color == 1 and Tablero[i - 1][j + 1].nombre == 'P') or 
    (entreLimites(i - 1, j - 1) and Tablero[i - 1][j - 1].color == 1 and Tablero[i - 1][j - 1].nombre == 'P'))) return true;


    // Rey negro
    if (color == 1 and (
    (entreLimites(i + 1, j + 1) and Tablero[i + 1][j + 1].color == 0 and Tablero[i + 1][j + 1].nombre == 'P') or 
    (entreLimites(i + 1, j - 1) and Tablero[i + 1][j - 1].color == 0 and Tablero[i + 1][j - 1].nombre == 'P'))) return true;
    
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
        if (Tablero[x + k*aux1][y + k*aux2].nombre != ' ') return false;
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
    if (j == y + 2 and i == x + 1) return true;
    if (j == y + 2 and i == x - 1) return true;
    if (j == y - 2 and i == x + 1) return true;
    if (j == y - 2 and i == x - 1) return true;
    if (j == y + 1 and i == x + 2) return true;
    if (j == y - 1 and i == x + 2) return true;
    if (j == y + 1 and i == x - 2) return true;
    if (j == y - 1 and i == x - 2) return true;
    return false;
}

// Comprueba si se puede mover el peon a esa casilla
bool movPeon(const VVP& Tablero, int i, int j, int x, int y, bool captura_al_paso) {
    int color = Tablero[x][y].color;

    // No avanzar mas de 2 casillas
    if (abs(y - j) > 2) return false;

    // No capturar a mas de 1 casilla de distancia
    if (abs(x - i) > 1) return false;   

    // No retroceder
    if (color == 0 and y <= j) return false;
    if (color == 1 and y >= j) return false;

    // Avanzar
    if (x == i) if (Tablero[i][j].nombre != ' ') return false;
    
    // Avanzar 2 casillas
    if (abs(y - j) == 2) {
        if (x != i) return false;
        if (color == 0) {
            if (y != 6) return false;
            if (Tablero[i][j + 1].nombre != ' ') return false;
        }
        if (color == 1) {
            if (y != 1) return false;
            if (Tablero[i][j - 1].nombre != ' ') return false;
        }
    }
    
    // Capturar
    if (abs(y - j) == 1 and abs(x - i) == 1) {
        // Captura al paso
        if (captura_al_paso) return true;
        // Captura
        if (Tablero[i][j].nombre == ' ') return false;
    }
    
    return true;
}

// Comprueba si se puede mover el rey a esa casilla
bool movRey(const VVP& Tablero, int i, int j, int x, int y) {
    int color = Tablero[x][y].color;
    if (abs(i - x) == 2 and j - y == 0 and not Tablero[x][y].movido and not atacado(Tablero, 4, j, color)) {
        // Enroque corto
        if (i == 6 and not Tablero[7][j].movido and Tablero[5][j].nombre == ' ' and Tablero[6][j].nombre == ' ' and
        not atacado(Tablero, 5, j, color) and not atacado(Tablero, 6, j, color)) return true;
        // Enroque largo
        if (i == 2 and not Tablero[0][j].movido and Tablero[1][j].nombre == ' ' 
        and Tablero[2][j].nombre == ' ' and Tablero[3][j].nombre == ' ' and 
        not atacado(Tablero, 3, j, color) and not atacado(Tablero, 2, j, color)) return true;
    }
    if (i == x + 1 and j == y) return true;
    if (i == x - 1 and j == y) return true;
    if (i == x and j == y + 1) return true;
    if (i == x and j == y - 1) return true;
    if (i == x + 1 and j == y + 1) return true;
    if (i == x - 1 and j == y + 1) return true;
    if (i == x + 1 and j == y - 1) return true;
    if (i == x - 1 and j == y - 1) return true;
        
    return false;
}

void moverPieza(VVP& Tablero, int i, int j, int x, int y, bool captura_al_paso, char corona) {
    // Captura al paso
    if (captura_al_paso) {
        if (Tablero[x][y].color == 0) Tablero[i][j + 1] = {' ', -1, false};
        if (Tablero[x][y].color == 1) Tablero[i][j - 1] = {' ', -1, false};
    }

    else if (Tablero[x][y].nombre == 'K' and abs(x - i) == 2) {
        // Enroque corto
        if (i == 6) {
            Tablero[5][j] = Tablero[7][j];
            Tablero[7][j] = {' ', -1, false};
        }

        // Enroque largo
        else if (i == 2) {
            Tablero[3][j] = Tablero[0][j];
            Tablero[0][j] = {' ', -1, false};
        }
    }

    // Movimiento estandar
    Tablero[i][j] = Tablero[x][y];
    Tablero[i][j].movido = true;
    Tablero[x][y] = {' ', -1, false};

    // Coronar peones
    if (Tablero[i][j].nombre == 'P' and (j == 0 or j == 7)) Tablero[i][j].nombre = corona;
}

// Recrea el movimiento y comprueba que el rey no este en jaque
bool jaque(VVP Tablero, int i, int j, int x, int y, bool captura_al_paso, int turno, char corona) {
    moverPieza(Tablero, i, j, x, y, captura_al_paso, corona);
    for (int k = 0; k < 8; ++k) {
        for (int l = 0; l < 8; ++l) {
            if (Tablero[k][l].nombre == 'K' and Tablero[k][l].color == turno) {
                if (atacado(Tablero, k, l, turno)) return true;
                return false;
            }
        }
    }
    return false;
}

bool posicionValida(const VVP& Tablero, int i, int j, int x, int y, bool captura_al_paso, int turno, char corona) {
    // Fuera de los limites
    if (not entreLimites(i, j)) return false;

    // Compueba que en la posicion inicial haya una pieza
    if (Tablero[x][y].nombre == ' ') return false;

    // No comer mismo color
    if (Tablero[x][y].color == Tablero[i][j].color) return false;

    // No mover fuera de turno
    if (Tablero[x][y].color != turno) return false;

    // Peon
    if (Tablero[x][y].nombre == 'P' and not movPeon(Tablero, i, j, x, y, captura_al_paso)) return false;
    
    // Caballo
    else if (Tablero[x][y].nombre == 'N' and not movCaballo(i, j, x, y)) return false;

    // Alfil
    else if (Tablero[x][y].nombre == 'B' and  not movAlfil(Tablero, i, j, x, y)) return false;

    // Torre
    else if (Tablero[x][y].nombre == 'R' and not movTorre(Tablero, i, j, x, y)) return false;

    // Reina
    else if (Tablero[x][y].nombre == 'Q' and not movAlfil(Tablero, i, j, x, y) and not movTorre(Tablero, i, j, x, y)) return false;

    // Rey
    else if (Tablero[x][y].nombre == 'K' and not movRey(Tablero, i, j, x, y)) return false;

    // No ignorar jaque
    if (jaque(Tablero, i, j, x, y, captura_al_paso, turno, corona)) return false;

    return true;
}

bool existeMovimientoValido(const VVP& Tablero, int turno, char pieza_prev, int aux_j, int aux_x, int aux_y) {
    // Encuentra una pieza aliada
    for (int x = 0; x < 8; ++x) {
        for (int y = 0; y < 8; ++y) {
            if (Tablero[x][y].color == turno) {
                // Encuentra un movimento valido para la pieza
                for (int i = 0; i < 8; ++i) {
                    for (int j = 0; j < 8; ++j) {
                        bool captura_al_paso = enPassant(i, j, x, y, pieza_prev, aux_j, aux_x, aux_y);
                        if (posicionValida(Tablero, i, j, x, y, captura_al_paso, turno, ' ')) return true;
                    }
                }
            }
        }
    }
    return false;
}

bool mate(const VVP& Tablero, int turno, char pieza_prev, int aux_j, int aux_x, int aux_y) {
    // Encuentra el rey
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            if (Tablero[i][j].nombre == 'K' and Tablero[i][j].color == turno) {
                // Comprueba si el rey esta en jaque
                if (atacado(Tablero, i, j, turno)) {
                    // Comprueba si existe un movimiento valido para el jugador actual
                    if (not existeMovimientoValido(Tablero, turno, pieza_prev, aux_j, aux_x, aux_y)) {
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

    int turno = 0;  // 0 = blancas, 1 = negras

    // Guarda la jugada anterior (para la captura al paso)
    int auxpos_yi, auxpos_y, auxpos_xi;
    auxpos_yi = auxpos_y = auxpos_xi = 0;
    char pieza_prev = ' ';

    char cpos_xi, cpos_yi, cpos_x, cpos_y;
    while (cin >> cpos_xi >> cpos_yi >> cpos_x >> cpos_y) {
        // Convertir caracter a entero
        int pos_xi = int(cpos_xi - 'a');
        int pos_x = int(cpos_x - 'a');
        int pos_yi = 8 - int(cpos_yi - '0');
        int pos_y = 8 - int(cpos_y - '0');

        char corona = ' ';
        bool captura_al_paso = false;
        if (entreLimites(pos_x, pos_y)) {
            // Introducir pieza a la que se quiere coronar
            if ((pos_y == 0 or pos_y == 7) and 
            Tablero[pos_xi][pos_yi].nombre == 'P' and movPeon(Tablero, pos_x, pos_y, pos_xi, pos_yi, false)) corona = coronarPeones();

            // Comprueba si hay captura al paso
            if (enPassant(pos_x, pos_y, pos_xi, pos_yi, pieza_prev, auxpos_y, auxpos_xi, auxpos_yi)) captura_al_paso = true;
        }
        if (posicionValida(Tablero, pos_x, pos_y, pos_xi, pos_yi, captura_al_paso, turno, corona)) {
            moverPieza(Tablero, pos_x, pos_y, pos_xi, pos_yi, captura_al_paso, corona);
            imprimirTablero(Tablero);

            // Cambia el turno
            turno = 1 - turno;

            // Actualiza la jugada anterior (para la captura al paso)
            pieza_prev = Tablero[pos_x][pos_y].nombre;
            auxpos_yi = pos_yi;
            auxpos_y = pos_y;
            auxpos_xi = pos_xi;

            // Comprueba si hay mate
            if (mate(Tablero, turno, pieza_prev, auxpos_y, auxpos_xi, auxpos_yi)) {
                cout << "¡Jaque mate! ";
                if (turno == 0) cout << "Las negras ganan." << endl;
                else cout << "Las blancas ganan." << endl;
                break;
            }
        }
        else cout << "Posicion invalida" << endl;
    }
}