# 💾 Emulador de Arquitectura de CPU Simple (16 bits)

Este proyecto implementa un emulador básico para una arquitectura de CPU simple de 16 bits en lenguaje C, junto con un ensamblador rudimentario en Python para convertir programas `.asm` en el formato de memoria que el emulador puede cargar.

## 🛠️ Arquitectura Simulada

El emulador simula una CPU con las siguientes características:

### ⚙️ Registros Principales
* **ACC** (Accumulator, 16 bits): Registro principal para operaciones aritméticas/lógicas.
* **X** (Index Register, 16 bits): Registro índice para direccionamiento.
* **PC** (Program Counter, 16 bits): Apunta a la siguiente instrucción a ejecutar.
* **Memoria (RAM)**: $4096$ palabras de $16$ bits (`uint16_t`).

### 🚩 Registro de Estado (`Status`)
Es un registro de 6 bits que contiene los siguientes *flags*:
* **Z** (Zero Flag): Activado si el resultado de la operación es cero.
* **N** (Negative Flag): Activado si el resultado es negativo (en complemento a 2).
* **C** (Carry Flag): Activado si hay acarreo en operaciones aritméticas.
* **I** (Interrupt Flag): Habilita/deshabilita interrupciones.
* **V** (Overflow Flag): Activado si hay desbordamiento aritmético.
* **H** (Halt Flag): Se activa cuando la CPU se detiene (instrucción halt).

### 📜 Formato de Instrucción (16 bits)
El emulador utiliza el siguiente formato para la instrucción:

| Bits | 15-13 | 12-9 | 8 | 7-6 | 5-0 |
| :---: | :---: | :---: | :---: | :---: | :---: |
| **Campo** | No usado | **OPCODE** | **R** | **DIRM** | **CD** |
| **Longitud** | 3 bits | 4 bits | 1 bit | 2 bits | 6 bits |

* **OPCODE (4 bits)**: Código de la operación (bits 9-12).
* **R (1 bit)**: Selector de Registro (0=X, 1=ACC) (bit 8).
* **DIRM (2 bits)**: Modo de Direccionamiento (bits 6-7).
* **CD (6 bits)**: Constante de Dirección (bits 0-5).

### Addressing Modes (Modos de Direccionamiento)
| DIRM | Nombre | Descripción |
| :---: | :---: | :--- |
| **00** | Directo | Dirección Efectiva (EA) = CD |
| **01** | Indirecto | EA = contenido de `mem[CD]` |
| **10** | Indexado | EA = CD + Registro X |
| **11** | Indirecto Indexado | EA = contenido de `mem[CD + X]` |

### 📋 Conjunto de Instrucciones
| Opcode | Mnemónico | Función Ejecutora | Descripción |
| :---: | :---: | :---: | :--- |
| **0** | `ST` | `store_data` | Store (Almacena el registro en memoria) |
| **1** | `LD` | `load_data` | Load (Carga de memoria a un registro) |
| **2** | `ADD` | `add_data` | Add (Suma el valor de memoria al registro) |
| **3** | `BR` | `branch_jump` | Branch (Salto Incondicional: `pc = eff_addr`) |
| **4** | `BZ` | `branch_if_zero` | Branch if Zero (Salto Condicional si Z=1) |
| **5** | `CLR` | `clear_reg` | Clear (Pone a cero un registro) |
| **6** | `DEC` | `decrement_reg` | Decrement (Decrementa un registro en 1) |
| **7** | **Extendida** | *N/A* | Usado para instrucciones sin operando, usa bits 7-8 |

**Instrucciones Extendidas (Opcode 7)**
| Ext. Opcode | Mnemónico | Función Ejecutora | Descripción |
| :---: | :---: | :---: | :--- |
| **0** | `HALT` | `halt_cpu` | Detiene la CPU (pone H=1) |
| **1** | `EI` | `enable_int` | Enable Interrupts (pone I=1) |
| **2** | `DI` | `disable_int` | Disable Interrupts (pone I=0) |

---

## 🚀 Uso y Compilación

### 1. Compilar el Emulador
Necesitas un compilador C (como GCC) para compilar el código fuente del emulador (`emulador.c`).

```bash
gcc emulador.c -o emulador
````

### 2\. Ensamblar un Programa

El ensamblador (`ensamblador.py`) convierte programas `.asm` en el formato de memoria que el emulador puede cargar.

**Ejemplo:** Ensamblar `suma_v1.asm`

```bash
python ensamblador.py suma_v1.asm
```

Esto generará un archivo de salida llamado `suma_v1.bin`.

### 3\. Ejecutar el Emulador

Ejecuta el emulador pasando el archivo ensamblado (`.bin`) como argumento.

```bash
./emulador suma_v1.bin
```

El emulador entrará en un **bucle de depuración (debug loop)**, mostrando el estado completo de la CPU (registros, flags y memoria) después de cada instrucción. Presiona **ENTER** para avanzar a la siguiente instrucción.

-----

## 📂 Programas de Ejemplo

Se incluyen varios archivos `.asm` para probar la funcionalidad:

  * **`suma_v1.asm`**: Carga los valores de `mem[5]`, `mem[6]`, y `mem[7]`, los suma en el acumulador (ACC), y guarda el resultado en `mem[8]`.
  * **`resta.asm`**: Demuestra la resta calculando $8 - 2 = 6$. Carga 8 de `mem[10]`, suma el complemento a dos de 2 (`0xFFFE`) de `mem[11]`, y guarda el resultado en `mem[12]`.

-----

## 🐍 Ensamblador (`ensamblador.py`)

El *script* de Python es una herramienta simple para convertir el código mnemónico a códigos binarios (formato de 16 bits) que el emulador puede cargar.

### Características del Ensamblador:

1.  Convierte instrucciones básicas (LD, ADD, ST, etc.) al formato binario de 16 bits, asumiendo el registro **ACC** y direccionamiento **Directo**.
2.  Procesa directivas de datos tipo `mem[DIRECCION] = VALOR` y las añade al archivo de salida ordenadas por dirección.
3.  Genera el código en formato decimal o hexadecimal de C para su fácil carga (ejemplo: `0x40A, // ADD ACC,[10]`).

-----

## 💻 Ejemplos de Código Ensamblador

A continuación, se muestran ejemplos de cómo se escriben los programas en la sintaxis de la herramienta:

### Ejemplo 1: Suma de Valores (`suma_v1.asm`)

Este programa carga tres valores de memoria (`1`, `2`, `4`), los suma en el acumulador (`ACC`), y guarda el resultado (`7`) en la posición `mem[8]`.

```asm
LD ACC,[5]    // Cargar 1 (mem[5]) a ACC. ACC = 1
ADD ACC,[6]   // Sumar 2 (mem[6]) a ACC. ACC = 3
ADD ACC,[7]   // Sumar 4 (mem[7]) a ACC. ACC = 7
ST ACC,[8]    // Almacenar el resultado de ACC (7) en mem[8]
HALT          // Detener la ejecución

; Directivas de datos
; Los datos se colocan después de las instrucciones
mem[5] = 1    ; Primer sumando
mem[6] = 2    ; Segundo sumando
mem[7] = 4    ; Tercer sumando
mem[8] = 0    ; Espacio para el resultado (inicializado en 0)
```

### Ejemplo 2: Resta usando Complemento a Dos (`resta.asm`)

Este programa realiza la operación de resta $8 - 2 = 6$. La resta se implementa sumando el complemento a dos del sustraendo. El valor `0xFFFE` (65534 decimal) es el complemento a dos de $-2$ en un registro de 16 bits.

```asm
; Programa: calcula 8 - 2 = 6
LD ACC,[10]    // Cargar 8 (mem[10]) a ACC. ACC = 8
ADD ACC,[11]   // Sumar -2 (mem[11]) a ACC. ACC = 6 (8 + 0xFFFE = 0x10006, con acarreo descartado)
ST ACC,[12]    // Guardar resultado (6) en mem[12]
HALT           // Detener CPU

; Directivas de datos
mem[10] = 8        ; Minuendo
mem[11] = 0xFFFE   ; Complemento a dos de 2 (-2)
mem[12] = 0        ; Espacio para el resultado
```