#include <iostream>
#include <ncurses.h>
#include <vector>
using namespace std;

// Definir pares de colores
#define BLUE_WHITE_PAIR 1
#define BLUE_BLACK_PAIR 2
#define RED_WHITE_PAIR 3
#define RED_BLACK_PAIR 4
#define WHITE_BLACK_PAIR 5
#define BLACK_GREEN_PAIR 6

struct Pieza {
    char nombre;
    int color; // Blanco 0, negro 1, vacio -1
    bool movido; // Se utiliza para el enroque
};

typedef vector<Pieza> VP;
typedef vector<VP> VVP;

typedef vector<bool> VB;
typedef vector<VB> VVB;

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

void initColors() {
    start_color();
    init_pair(BLUE_WHITE_PAIR, COLOR_BLUE, COLOR_WHITE);
    init_pair(BLUE_BLACK_PAIR, COLOR_BLUE, COLOR_BLACK);
    init_pair(RED_WHITE_PAIR, COLOR_RED, COLOR_WHITE);
    init_pair(RED_BLACK_PAIR, COLOR_RED, COLOR_BLACK);
    init_pair(WHITE_BLACK_PAIR, COLOR_WHITE, COLOR_BLACK);
    init_pair(BLACK_GREEN_PAIR, COLOR_BLACK, COLOR_GREEN);
}

void imprimirTablero(const VVP& Tablero, int startx, int starty) {
    clear(); // Limpiar la pantalla

    for (int i = 0; i < 8; ++i) {
        mvprintw(starty + 2*i, startx - 3, " %d ", 8 - i);
        for (int j = 0; j < 8; ++j) {
            if (Tablero[j][i].color == 0) {
                if ((i + j)%2 == 0) attron(COLOR_PAIR(BLUE_WHITE_PAIR));
                else attron(COLOR_PAIR(BLUE_BLACK_PAIR));
            }
            else {
                if ((i + j)%2 == 0) attron(COLOR_PAIR(RED_WHITE_PAIR));
                else attron(COLOR_PAIR(RED_BLACK_PAIR));
            }

            mvprintw(starty + 2*i, startx + 3*j, " %c ", Tablero[j][i].nombre);

            attroff(COLOR_PAIR(BLUE_WHITE_PAIR) | COLOR_PAIR(BLUE_BLACK_PAIR) | COLOR_PAIR(RED_WHITE_PAIR) | COLOR_PAIR(RED_BLACK_PAIR));
        }
        for (int j = 0; j <= 8; ++j) mvprintw(starty + i*2, startx + j*3, "|");
    }
    for (int i = 0; i <= 8; ++i) for (int j = 0; j <= 24; ++j) mvprintw(starty - 1 + i*2, startx + j, "%c", '-');
    for (int i = 0; i < 8; ++i) mvprintw(starty + 16, startx + i*3 + 2, "%c", 'A' + i);

    refresh(); // Actualizar la pantalla    
}

// Muestra un mensaje por tiempo milisegundos y despues lo elimina
void imprimirMensaje(string s, int tiempo) {
    int height, width;
    getmaxyx(stdscr, height, width);
    int x = (width - s.length()) / 2;

    // Mostrar el mensaje
    attron(COLOR_PAIR(WHITE_BLACK_PAIR));
    mvprintw(height - 1, x, s.c_str());
    attroff(COLOR_PAIR(WHITE_BLACK_PAIR));
    refresh();

    // Esperar a que el usuario presione una tecla o esperar 2 segundos
    timeout(tiempo);  
    getch();  
    timeout(-1);  // Desactivar el tiempo de espera

    // Borrar el mensaje
    mvprintw(height - 1, x, string(s.size(), ' ').c_str());
    refresh(); 
}

char coronarPeones() {
    // Coronar peones
    imprimirMensaje("Introduce la pieza a la que quieres coronar (Q, R, B, N): ", 4000);
    char corona;
    cin >> corona;
    while (corona != 'Q' and corona != 'R' and corona != 'B' and corona != 'N') cin >> corona;
    return corona;
}

// Comprueba si hay captura al paso
bool enPassant(int i, int j, int x, int y, char pieza_prev, int aux_j, int aux_x, int aux_y, int color) {
    // Peon
    if (pieza_prev != 'P') return false;
    // Ultimo movimiento de 2 de distancia
    if (abs(aux_y - aux_j) != 2) return false;
    // Quinta fila
    if (color == 0 and y != 3) return false;
    if (color == 1 and y != 4) return false;
    // Misma fila de destino 
    if (aux_x != i) return false;

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
bool movPeon(const VVP& Tablero, int i, int j, int x, int y, char pieza_prev, int aux_j, int aux_x, int aux_y, bool& captura_al_paso) {
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
        captura_al_paso = enPassant(i, j, x, y, pieza_prev, aux_j, aux_x, aux_y, color);
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

void moverPieza(VVP& Tablero, int i, int j, int x, int y, char corona) {
    // Captura al paso
    if (Tablero[x][y].nombre == 'P' and x != i and Tablero[i][j].nombre == ' ') {
        if (Tablero[x][y].color == 0) Tablero[i][j + 1] = {' ', -1, false};
        if (Tablero[x][y].color == 1) Tablero[i][j - 1] = {' ', -1, false};
    }

    // Enroque
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
bool jaque(VVP Tablero, int i, int j, int x, int y, int turno) {
    moverPieza(Tablero, i, j, x, y, 'Q');
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

void mostrarPosicionesValidas(const VVB& PV, const VVP& Tablero, int startx, int starty) {
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            if (PV[j][i]) {
                attron(COLOR_PAIR(BLACK_GREEN_PAIR));
                if (Tablero[j][i].nombre == ' ') mvprintw(starty + i*2, startx + j*3 + 1, " ");
                else mvprintw(starty + i*2, startx + j*3 + 1, "%c", Tablero[j][i].nombre); 
                mvprintw(starty + i*2, startx + j*3 + 2, " ");
                attroff(COLOR_PAIR(BLACK_GREEN_PAIR));
                refresh();
            }
        }
    }
}

VVB calcularPosicionesValidas(const VVP& Tablero, int x, int y, char pieza_prev, int aux_j, int aux_x, int aux_y, int turno) {
    VVB PV(8, VB(8, false));
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            bool valida = false;
            bool captura_al_paso = false;
            if (Tablero[x][y].nombre == 'P' and movPeon(Tablero, i, j, x, y, pieza_prev, aux_j, aux_x, aux_y, captura_al_paso)) valida = true;
            if (Tablero[x][y].nombre == 'N' and movCaballo(i, j, x, y)) valida = true;
            if (Tablero[x][y].nombre == 'B' and movAlfil(Tablero, i, j, x, y)) valida = true;
            if (Tablero[x][y].nombre == 'R' and movTorre(Tablero, i, j, x, y)) valida = true;
            if (Tablero[x][y].nombre == 'Q' and (movAlfil(Tablero, i, j, x, y) or movTorre(Tablero, i, j, x, y))) valida = true;
            if (Tablero[x][y].nombre == 'K' and movRey(Tablero, i, j, x, y)) valida = true;
            if (Tablero[i][j].color == Tablero[x][y].color) valida = false;
            if (valida and not jaque(Tablero, i, j, x, y, turno)) PV[i][j] = true;
        }
    }
    return PV;
}

bool existeMovimientoValido(const VVP& Tablero, char pieza_prev, int aux_j, int aux_x, int aux_y, int turno) {
    // Encuentra una pieza aliada
    for (int x = 0; x < 8; ++x) {
        for (int y = 0; y < 8; ++y) {
            if (Tablero[x][y].color == turno) {
                bool captura_al_paso = false;
                char corona = 'Q';
                VVB PV = calcularPosicionesValidas(Tablero, x, y, pieza_prev, aux_j, aux_x, aux_y, turno);
                for (int i = 0; i < 8; ++i) {
                    for (int j = 0; j < 8; ++j) {
                        if (PV[i][j]) return true;
                    }
                }
            }
            
        }
    }
    return false;
}

bool mate(const VVP& Tablero, char pieza_prev, int aux_j, int aux_x, int aux_y, int turno) {
    // Encuentra el rey
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            if (Tablero[i][j].nombre == 'K' and Tablero[i][j].color == turno) {
                // Comprueba si el rey esta en jaque
                if (atacado(Tablero, i, j, turno)) {
                    // Comprueba si existe un movimiento valido para el jugador actual
                    if (not existeMovimientoValido(Tablero, pieza_prev, aux_j, aux_x, aux_y, turno)) {
                        return true;
                    }
                }
                return false;
            }
        }
    }
    return false;
}

void obtenerPosicionRaton(int& i, int& j, int startx, int starty) {
    MEVENT event;
    int input;
    while (true) {
        input = getch();
        if (input == KEY_MOUSE) {
            if (getmouse(&event) == OK) {
                int mouse_y = event.y;
                int mouse_x = event.x;
                // Verificar si el clic está dentro del tablero
                if (mouse_y >= starty and mouse_y < starty + 16 and mouse_x >= startx and mouse_x < startx + 24) {
                    i = (mouse_x - startx)/3;
                    j = (mouse_y - starty)/2;
                    break;
                }
            }
        }
    }
}

int main() {
    initscr(); // Inicializar la pantalla de NCurses
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    initColors();

    int height, width;
    getmaxyx(stdscr, height, width);
    int starty = (height - 8 * 2) / 2;
    int startx = (width - 8 * 3) / 2;

    VVP Tablero = crearTablero();
    imprimirTablero(Tablero, startx, starty);

    int turno = 0;  // 0 = blancas, 1 = negras

    // Guarda la jugada anterior (para la captura al paso)
    int auxpos_yi, auxpos_y, auxpos_xi;
    auxpos_yi = auxpos_y = auxpos_xi = 0;
    char pieza_prev = ' ';

    while (true) {
        // Obtiene las coordenadas iniciales
        int pos_x, pos_y, pos_xi, pos_yi;
        obtenerPosicionRaton(pos_xi, pos_yi, startx, starty);

        // Comprueba si hay una pieza que se pueda mover en esa posicion 
        if (Tablero[pos_xi][pos_yi].color == turno) {
            char corona = ' ';

            // Muestra las posiciones validas de la pieza seleccionada
            VVB PV = calcularPosicionesValidas(Tablero, pos_xi, pos_yi, pieza_prev, auxpos_y, auxpos_xi, auxpos_yi, turno);
            mostrarPosicionesValidas(PV, Tablero, startx, starty);

            // Obtiene las coordenadas finales
            obtenerPosicionRaton(pos_x, pos_y, startx, starty);

            // Comprueba si se quiere coronar un peon
            if (((pos_y == 0 and Tablero[pos_xi][pos_yi].color == 0) or (pos_y == 7 and Tablero[pos_xi][pos_yi].color == 1)) and 
            Tablero[pos_xi][pos_yi].nombre == 'P') corona = coronarPeones();

            // Comprueba si es una posicion valida
            if (PV[pos_x][pos_y]) {
                moverPieza(Tablero, pos_x, pos_y, pos_xi, pos_yi, corona);
                imprimirTablero(Tablero, startx, starty);

                // Cambia el turno
                turno = 1 - turno;

                // Actualiza la jugada anterior (para la captura al paso)
                pieza_prev = Tablero[pos_x][pos_y].nombre;
                auxpos_yi = pos_yi;
                auxpos_y = pos_y;
                auxpos_xi = pos_xi;

                // Comprueba si hay mate
                if (mate(Tablero, pieza_prev, auxpos_y, auxpos_xi, auxpos_yi, turno)) {
                    imprimirMensaje("¡Jaque mate! ", 2000);
                    if (turno == 0) imprimirMensaje("Las negras ganan.", 1000);
                    else imprimirMensaje("Las blancas ganan.", 1000);
                    endwin(); // Finalizar NCurses
                    break;
                }
            }
            else imprimirTablero(Tablero, startx, starty);
        }
    }
}