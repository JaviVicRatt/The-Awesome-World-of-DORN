# The Awesome World of DORN

## English

Native C migration of the DORN board game. The playable application uses SDL2
for the window, input, rendering, and audio; game rules are kept in a standalone
C library so they can be tested without starting the graphical application.

### Build

SDL2 and the image decoder are included under `native/vendor`. You need a
C/C++ compiler toolchain (C11 support) and CMake 3.20 or newer. On Linux, also
install the development headers for the window system available on your
distribution (X11 or Wayland). On Windows, use CMake with Visual Studio 2019 or
newer, or another supported SDL2 C compiler toolchain.

#### Linux

```sh
cmake -S native -B native/build -DCMAKE_BUILD_TYPE=Release
cmake --build native/build --parallel
ctest --test-dir native/build --output-on-failure
./native/build/dorn
```

The `dorn_sdl_smoke` CTest test initializes SDL, loads the assets, and renders
several frames using the dummy video/audio backend.

#### Windows (Visual Studio)

```powershell
cmake -S native -B native/build -G "Visual Studio 17 2022"
cmake --build native/build --config Release
ctest --test-dir native/build -C Release --output-on-failure
.\native\build\Release\dorn.exe
```

The build copies the existing game images and sound effects beside the
executable. The application can also read them from the original
`src/com/dorn/assets` directory when run from the project build folder.

### Controls

- Menu: Enter to choose heroes; Escape to quit.
- Hero selection: keys 1–9 toggle heroes, Enter starts with 1–5 heroes.
- Game: click a highlighted board space to move; click a target to attack;
  Enter advances the current phase/actor; Escape returns to the menu.

### Migration notes

The Java/NetBeans project files are retained as a reference during the
transition; they are not used to compile or run the native application. The SDL2
prototype displays the original map and uses explicit cells and links for the
Argos, stone path, forest, mine, throne, and herbalist spaces defined in the
Java map source. Movement is calculated over those links, so a visual diagonal
between square mine spaces does not create a move. Heroes may pass one another
only on the ochre path spaces; monsters cannot enter those spaces. Attack
distance uses the cell graph, and pike attacks require exactly two spaces.

This is still a prototype, not a complete rules implementation. The Java map
leaves entire sectors and their links unfinished; sectors without verified
coordinates and links, including the Ritual Chamber, are omitted from the
interactive graph. The manual describes summoning, line of sight, the Doom
Gate, and artifacts, but the map implementation does not provide enough cell
and obstacle data to implement those rules faithfully. To avoid fabricating
rules, summoning is disabled with an explicit status message, and isolated
initial-spawn cells remain non-traversable. Those areas and rules need verified
board data before they can be enabled.

### Third-party code

- SDL2 2.30.11: zlib license, source in `native/vendor/SDL`.
- `stb_image`: public domain or MIT, source in `native/vendor/stb_image.h`.

See the bundled license texts and `native/THIRD_PARTY.md`.

## Español

Migración nativa en C del juego de mesa DORN. La aplicación jugable utiliza
SDL2 para la ventana, la entrada, el renderizado y el audio; las reglas del
juego se mantienen en una biblioteca C independiente para poder probarlas sin
iniciar la aplicación gráfica.

### Compilación

SDL2 y el decodificador de imágenes están incluidos en `native/vendor`. Se
necesita una cadena de herramientas C/C++ compatible con C11 y CMake 3.20 o
posterior. En Linux, instala también los encabezados de desarrollo del sistema
de ventanas disponible en tu distribución (X11 o Wayland). En Windows, usa
CMake con Visual Studio 2019 o posterior, u otra cadena de herramientas C
compatible con SDL2.

#### Linux

```sh
cmake -S native -B native/build -DCMAKE_BUILD_TYPE=Release
cmake --build native/build --parallel
ctest --test-dir native/build --output-on-failure
./native/build/dorn
```

La prueba `dorn_sdl_smoke` de CTest inicializa SDL, carga los recursos y renderiza
varios fotogramas usando los controladores ficticios de vídeo y audio.

#### Windows (Visual Studio)

```powershell
cmake -S native -B native/build -G "Visual Studio 17 2022"
cmake --build native/build --config Release
ctest --test-dir native/build -C Release --output-on-failure
.\native\build\Release\dorn.exe
```

La compilación copia las imágenes y los efectos de sonido existentes junto al
ejecutable. La aplicación también puede leerlos desde el directorio original
`src/com/dorn/assets` cuando se ejecuta desde la carpeta de compilación del
proyecto.

### Controles

- Menú: Enter permite elegir héroes; Escape cierra la aplicación.
- Selección de héroes: las teclas 1–9 activan o desactivan héroes; Enter inicia
  la partida con entre 1 y 5 héroes.
- Partida: haz clic en una casilla resaltada para moverte y en un objetivo para
  atacar; Enter avanza la fase o el actor actual; Escape vuelve al menú.

### Notas de migración

Los archivos del proyecto Java/NetBeans se conservan como referencia durante
la transición; no se utilizan para compilar ni ejecutar la aplicación nativa.
El prototipo SDL2 muestra el mapa original y usa casillas y enlaces explícitos
para Argos, el camino de piedra, el bosque, la mina, el trono y el herbolario,
definidos en el mapa Java. El movimiento se calcula sobre esos enlaces, por lo
que una diagonal dibujada entre casillas cuadradas de la mina no crea un paso.
Los héroes solo pueden atravesarse entre sí en las casillas ocres del camino;
los monstruos no pueden entrar en ellas. La distancia de ataque utiliza el
grafo de casillas y los ataques con lanza requieren exactamente dos casillas.

Este proyecto sigue siendo un prototipo y no implementa todas las reglas. El
mapa Java deja sectores enteros y sus enlaces sin terminar; los sectores sin
coordenadas y enlaces verificados, incluida la Cámara Ritual, se omiten del
grafo interactivo. El manual describe la invocación, la línea de visión, la
Puerta de la Condena y los artefactos, pero el mapa no proporciona suficientes
datos de casillas y obstáculos para implementar esas reglas fielmente. Para no
inventar reglas, la invocación está desactivada y se muestra un mensaje de
estado; las casillas aisladas de aparición inicial tampoco son transitables.
Esas zonas y reglas requieren datos verificados del tablero antes de poder
activarse.

### Código de terceros

- SDL2 2.30.11: licencia zlib; código fuente en `native/vendor/SDL`.
- `stb_image`: dominio público o licencia MIT; código fuente en
  `native/vendor/stb_image.h`.

Consulta los textos de licencia incluidos y `native/THIRD_PARTY.md`.
