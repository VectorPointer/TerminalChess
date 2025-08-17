# ♟️ Ajedrez en Terminal con Soporte de Ratón  

Este proyecto implementa un juego de **ajedrez jugable en la terminal** utilizando **C++**.  
El tablero se renderiza en la consola con caracteres Unicode y colores ANSI, y permite mover piezas **usando clics y arrastres con el ratón** (soporte para xterm y terminales compatibles).  

---

## ✨ Características  

- Renderizado del tablero en consola con **colores ANSI** y símbolos Unicode.  
- Movimientos de piezas mediante **clic de ratón o arrastrar/soltar**.  
- Reglas completas del ajedrez:  
  - Movimientos válidos para todas las piezas.  
  - Captura al paso (`en passant`).  
  - Enroque (corto y largo).  
  - Coronación de peones (con selección de pieza).  
- Detección de **jaque** y **jaque mate**.  
- Mensajes dinámicos en la línea de estado (con temporizador).  
- Control de turnos entre **blancas y negras**.  

---

## 📦 Requisitos  

- **Linux / macOS** (requiere soporte ANSI y captura de ratón en terminal).  
- Compilador **C++17** o superior.  
- Terminal compatible con **xterm mouse tracking**:  
  - `xterm`  
  - `gnome-terminal`  
  - `konsole`  
  - `alacritty`  

---

## 🔧 Compilación  

Compila el programa con:  

```bash
g++ -std=c++17 -pthread -o ajedrez main.cc
