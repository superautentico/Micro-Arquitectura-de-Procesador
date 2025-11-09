#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MEM_SIZE 4096


// DEFINICIONES DE REGISTROS Y ESTADOS
// ===================================

/*
 Registro de Estado (Flags) - Cada flag es 1 bit
 z: Zero Flag - Se activa (1) cuando el resultado de una operación es cero
 n: Negative Flag - Se activa cuando el resultado es negativo (en complemento a 2)
 c: Carry Flag - Se activa cuando hay acarreo en operaciones aritméticas
 i: Interrupt Flag - Habilita/deshabilita interrupciones
 v: Overflow Flag - Se activa cuando hay desbordamiento aritmético
 h: Halt Flag - Se activa cuando la CPU se detiene (instrucción halt)
 */
typedef struct
{
    uint8_t z : 1;  
    uint8_t n : 1;   
    uint8_t c : 1;  
    uint8_t i : 1;  
    uint8_t v : 1;  
    uint8_t h : 1;  
} Status;

/*
 Estructura principal de la CPU - Simula un procesador simple
 mem: Memoria principal, en nuestro caso un array de N palabras de 16 bits según definamos MEM_SIZE
 acc: Accumulator, registro acumulador para operaciones aritméticas
 x: Index Register, registro índice para direccionamiento
 pc: Program Counter, contador de programa, apunta a la siguiente instrucción
 status: Estructura de registro de estado con los flags de la CPU
 */
typedef struct
{
    uint16_t mem[MEM_SIZE];
    uint16_t acc;          
    uint16_t x;            
    uint16_t pc;           
    Status status;         
} CPU;


// CONSTANTES PARA DECODIFICACIÓN DE INSTRUCCIONES
// ===============================================

/*
FORMATO DE INSTRUCCIÓN (16 bits):

Bits 15-13: No usados
Bits 12-9:  Código de operación (opcode)
Bit 8:      Selector de registro (0=X, 1=ACC)
Bits 7-6:   Modo de direccionamiento (dirm)
Bits 5-0:   Constante de dirección (cd)

[000][OPCO][R][DI][CDCDCDCDCD]

Para instrucciones extendidas (opcode=7):
Bits 7-8 se usan como extended opcode
*/

#define OPCODE_SHIFT 9    // Desplazamiento para extraer opcode (bits 9-11)
#define OPCODE_MASK 0x7   // Máscara para opcode (3 bits: 0-7)
#define EXT_SHIFT 7       // Desplazamiento para extended opcode (bits 7-8)  
#define EXT_MASK 0x3      // Máscara para extended opcode (2 bits: 0-3)


// CONTEXTO DE INSTRUCCIÓN - PARA EJECUCIÓN EN DOS FASES
// =====================================================

/*
  Contexto de Instrucción - Contiene toda la información decodificada
  de una instrucción para separar fetch/decode de execute
*/
typedef struct {
    uint8_t opcode;        // Código de operación principal (bits 9-11)
    uint8_t reg;           // Registro seleccionado (0=X, 1=ACC)
    uint8_t addr_mode;     // Modo de direccionamiento (bits 6-7)
    uint16_t address;      // Constante de dirección (bits 0-5)
    uint16_t eff_addr;     // Dirección Efectiva - dirección real en memoria
    uint8_t is_extended;   // Flag: 1 si es instrucción extendida
    uint8_t ext_opcode;    // Extended opcode (para opcode=7, bits 7-8)
} InstructionContext;


void store_data(CPU *cpu, uint8_t reg, uint16_t data);
void load_data(CPU *cpu, uint8_t reg, uint16_t data);
void add_data(CPU *cpu, uint8_t reg, uint16_t data);
void branch_jump(CPU *cpu, uint8_t reg, uint16_t data);
void branch_if_zero(CPU *cpu, uint8_t reg, uint16_t data);
void clear_reg(CPU *cpu, uint8_t reg, uint16_t data);
void decrement_reg(CPU *cpu, uint8_t reg, uint16_t data);
void halt_cpu(CPU *cpu, uint8_t reg, uint16_t data);
void enable_int(CPU *cpu, uint8_t reg, uint16_t data);
void disable_int(CPU *cpu, uint8_t reg, uint16_t data);


// TABLAS DE INSTRUCCIONES
// =======================

/*
Estructura Instruction - mapea nombres a funciones ejecutoras
name: Nombre mnemónico de la instrucción
execute: Puntero a la función que implementa la instrucción
 */
typedef struct {
    char *name;                              // Nombre de la instrucción
    void (*execute)(CPU *cpu, uint8_t reg, uint16_t data);  // Función ejecutora
} Instruction;


/*
Tabla de instrucciones normales (opcodes 0-6)
Índice: número de opcode
Contenido: {nombre, función ejecutora}
*/
Instruction instruction_set[] = {
    {"st", store_data},    // Opcode 0: Store
    {"ld", load_data},     // Opcode 1: Load  
    {"add", add_data},     // Opcode 2: Add
    {"br", branch_jump},   // Opcode 3: Branch
    {"bz", branch_if_zero},// Opcode 4: Branch if Zero
    {"clr", clear_reg},    // Opcode 5: Clear
    {"dec", decrement_reg} // Opcode 6: Decrement
};

/*
 Tabla de instrucciones extendidas (opcode 7 + extended opcode)  
 Índice: número de extended opcode
 Contenido: {nombre, función ejecutora}
 */
Instruction extended_set[] = {
    {"halt", halt_cpu},    // Extended 0: Halt
    {"ei", enable_int},    // Extended 1: Enable Interrupts
    {"di", disable_int}    // Extended 2: Disable Interrupts
};


// IMPLEMENTACIÓN DE LAS INSTRUCCIONES
// ===================================

/*
 ST - STORE: Almacena el valor de un registro en memoria
 - cpu Puntero a la estructura CPU
 - reg Registro fuente (0=X, 1=ACC)
 - eff_addr Dirección de memoria destino

 mem[eff_addr] = registro
 No afecta a ningún flag
*/
void store_data(CPU *cpu, uint8_t reg, uint16_t eff_addr) {
    if (reg) {
        cpu->mem[eff_addr] = cpu->acc;  // Almacena ACC en memoria
    } else {
        cpu->mem[eff_addr] = cpu->x;    // Almacena X en memoria
    }
}

/*
LD - LOAD: Carga un valor de memoria a un registro
Afecta al flag Z, basado en el valor cargado
*/
void load_data(CPU *cpu, uint8_t reg, uint16_t eff_addr) {
    if (reg) {
        cpu->acc = cpu->mem[eff_addr];  // Carga memoria en ACC
        cpu->status.z = (cpu->acc == 0);  // Actualiza flag Z basado en ACC
    } else {
        cpu->x = cpu->mem[eff_addr];    // Carga memoria en X
        cpu->status.z = (cpu->x == 0);    // Actualiza flag Z basado en X
    }
}

/*
ADD - ADD: Suma un valor de memoria a un registro
registro = registro + mem[eff_addr]
Afecta al flag Z, basado en el valor cargado
*/
void add_data(CPU *cpu, uint8_t reg, uint16_t eff_addr) {
    if (reg) {
        cpu->acc += cpu->mem[eff_addr];  // Suma a ACC
        cpu->status.z = (cpu->acc == 0);
    } else {
        cpu->x += cpu->mem[eff_addr];    // Suma a X
        cpu->status.z = (cpu->x == 0);
    }
}

/*
BR - BRANCH: Salto incondicional a una dirección
pc = eff_addr
NOTA: Decrementamos pc porque después se incrementa en el ciclo principal
*/
void branch_jump(CPU *cpu, uint8_t reg, uint16_t eff_addr) {
    cpu->pc = eff_addr;      // Salto a la dirección especificada
    cpu->pc--;         // Compensación por el incremento posterior en el ciclo
}

/*
BZ - BRANCH IF ZERO: Salto condicional si Z flag está activo
Operación ==> if (Z) then pc = eff_addr
*/
void branch_if_zero(CPU *cpu, uint8_t reg, uint16_t eff_addr) {
    if (cpu->status.z) { 
        cpu->pc = eff_addr;    
        cpu->pc--;          // Compensación por el incremento posterior
    }
}

/*
CLR - CLEAR: Pone a cero un registro
Z siempre se activa porque el resultado es 0
*/
void clear_reg(CPU *cpu, uint8_t reg, uint16_t eff_addr) {
    if (reg) {
        cpu->acc = 0;      // Limpia ACC
    } else {
        cpu->x = 0;        // Limpia X
    }
    cpu->status.z = 1;     // Siempre activa Z flag (resultado es 0)
}

/*
DEC - DECREMENT: Decrementa un registro en 1
*/
void decrement_reg(CPU *cpu, uint8_t reg, uint16_t eff_addr) {
    if (reg) {
        cpu->acc--;                    // Decrementa ACC
        cpu->status.z = (cpu->acc == 0); // Actualiza Z flag
    } else {
        cpu->x--;                      // Decrementa X  
        cpu->status.z = (cpu->x == 0);   // Actualiza Z flag
    }
}

// INSTRUCCIONES EXTENDIDAS
// ========================

/*
HALT - DETENER: Detiene la ejecución de la CPU
*/
void halt_cpu(CPU *cpu, uint8_t reg, uint16_t eff_addr) {
    cpu->status.h = 1;    // Activa Halt Flag
    cpu->pc--;            // Mantiene pc en la misma instrucción
}

/*
EI - ENABLE INTERRUPTS: Habilita las interrupciones
*/
void enable_int(CPU *cpu, uint8_t reg, uint16_t eff_addr) {
    cpu->status.i = 1;    // Activa Interrupt Flag
}

/*
DI - DISABLE INTERRUPTS: Deshabilita las interrupciones  
*/
void disable_int(CPU *cpu, uint8_t reg, uint16_t eff_addr) {
    cpu->status.i = 0;    // Desactiva Interrupt Flag
}

// FUNCIONES PRINCIPALES DE LA CPU
// ===============================

/*
 resetCPU - Inicializa la CPU a estado conocido
 cpu Puntero a la estructura CPU a resetear

 - Limpia toda la memoria (pone a 0)
 - Pone todos los registros a 0
 - Pone pc a 0 (inicio del programa)
 - Desactiva todos los flags de estado
 */
void resetCPU(CPU *cpu)
{
    memset(cpu, 0, sizeof(CPU));
    cpu->pc = 0;                
}

/*
FASE 1: Obtiene y decodifica una instrucción
 cpu Puntero a la estructura CPU
 ctx Puntero al contexto donde almacenar la instrucción decodificada

 Esta función:
 1. Obtiene la instrucción de mem[pc]
 2. Decodifica todos los campos de la instrucción
 3. Calcula la dirección efectiva según el modo de direccionamiento
 4. Identifica si es instrucción extendida
 */
void fetch_and_decode(CPU *cpu, InstructionContext *ctx) {
    //FETCH - Obtener instrucción de la memoria en la posición pc
    uint16_t inst_code = cpu->mem[cpu->pc];
    
    //DECODE - Extraer todos los campos de la instrucción
    ctx->opcode = (inst_code >> OPCODE_SHIFT) & OPCODE_MASK;  // Bits 9-11
    ctx->reg = (inst_code >> 8) & 0x1;                        // Bit 8
    ctx->addr_mode = (inst_code >> 6) & 0x3;                  // Bits 6-7
    ctx->address = inst_code & 0x3F;                          // Bits 0-5
    
    //CALCULAR DIRECCIÓN EFECTIVA según modo de direccionamiento
    ctx->eff_addr = ctx->address;  // Por defecto: direccionamiento directo (addr_mode = 00)

    if (ctx->addr_mode == 0x1) { // Aqui estamos haciendo una comparacion de addr_mode con una mascara para ver
                            // si es igual a 01 que correspondería a modo indirecto en los bits 6-7
        // Modo Indirecto: EA = contenido de mem[address]
        ctx->eff_addr = cpu->mem[ctx->address];
    }
    else if (ctx->addr_mode == 0x2) {
        // Modo Indexado: EA = address + registro X
        ctx->eff_addr = ctx->address + cpu->x;
    }
    else if (ctx->addr_mode == 0x3) {
        // Modo Indirecto Indexado: EA = contenido de mem[address + X]
        ctx->eff_addr = cpu->mem[ctx->address + cpu->x];
    }
    
    //IDENTIFICAR INSTRUCCIÓN EXTENDIDA
    ctx->is_extended = (ctx->opcode == 7);
    if (ctx->is_extended) {
        ctx->ext_opcode = (inst_code >> EXT_SHIFT) & EXT_MASK;
    }
}

/*
FASE 2: Ejecuta una instrucción decodificada
 cpu Puntero a la estructura CPU

 Flujo:
 1. Crea contexto y llama a fetch_and_decode()
 2. Muestra información de depuración
 3. Reinicia Zero Flag (según convención)
 4. Ejecuta la instrucción usando las tablas
 5. Incrementa el contador de programa
 */
void execute_instruction(CPU *cpu)
{
    InstructionContext ctx;  // Contexto para esta instrucción
    
    //FETCH & DECODE
    fetch_and_decode(cpu, &ctx);
    
    // Mostrar información de depuración
    printf("DEBUG: op: %x, reg: %x, dirm: %x, cd: %x, ea: %x, data: %d\n", 
           ctx.opcode, ctx.reg, ctx.addr_mode, ctx.address, ctx.eff_addr, ctx.eff_addr);
    
    // Reiniciar Zero Flag al inicio de cada instrucción, debido a que algunas instrucciones podrian no modificarlo y eso conllevaria a errores
    cpu->status.z = 0;
    
    //EXECUTE - Usar tablas para ejecutar la instrucción correcta
    if (ctx.is_extended) {
        printf("Executing ext %s %x, %x\n", extended_set[ctx.ext_opcode].name, ctx.reg, ctx.eff_addr);
        extended_set[ctx.ext_opcode].execute(cpu, ctx.reg, ctx.eff_addr);
    } else {
        printf("Executing %s %x, %x\n", instruction_set[ctx.opcode].name, ctx.reg, ctx.eff_addr);
        instruction_set[ctx.opcode].execute(cpu, ctx.reg, ctx.eff_addr);
    }
    
    // Avanzar a la siguiente instrucción (a menos que instrucción modifique pc)
    cpu->pc++;
}

/*
 printCPUState - Muestra el estado actual de la CPU
 cpu Puntero a la estructura CPU

 Muestra:
 - Valores de registros principales (PC, X, ACC)
 - Estado de todos los flags
 - Primera parte de la memoria (30 palabras)
 */
void printCPUState(CPU *cpu)
{
    // Registros principales
    printf("PC:%x X:%x ACC:%x\n", cpu->pc, cpu->x, cpu->acc);
    
    // Flags de estado
    printf("STATUS: [Z:%x N:%x C:%x I:%x V:%x H:%x]\n", 
           cpu->status.z, cpu->status.n, cpu->status.c, 
           cpu->status.i, cpu->status.v, cpu->status.h);

    // Encontrar hasta dónde hay datos en memoria
    int max_used = 0;
    for (int i = MEM_SIZE - 1; i >= 0; i--) {
        if (cpu->mem[i] != 0) {
            max_used = i;
            break;
        }
    }
    
    // Mostrar memoria usada + margen (mínimo 20 palabras)
    int words_to_show = (max_used + 10 > 30) ? max_used + 10 : 30;
    if (words_to_show > MEM_SIZE) words_to_show = MEM_SIZE;
    
    printf("Memory [0-%d]: ", words_to_show - 1);
    for (int i = 0; i < words_to_show; i++)
    {
        printf("%x ", cpu->mem[i]);
        if (i % 10 == 9) {
            printf("\n");
            if (i < words_to_show - 1) printf("               ");
        }
    }
    printf("\n---\n");
}

/*
 cpu_loop - Bucle principal de ejecución de la CPU
 cpu Puntero a la estructura CPU

 Ciclo de ejecución:
 1. Ejecutar instrucción actual
 2. Mostrar estado
 3. Esperar entrada del usuario (para depuración)
 4. Repetir hasta que se active Halt Flag
 */
void cpu_loop(CPU *cpu)
{
    // Ejecutar hasta que se active el Halt Flag
    while (!cpu->status.h) {
        execute_instruction(cpu);  // Ejecutar una instrucción
        printCPUState(cpu);        // Mostrar estado resultante
        getchar();                 // Pausa
    }
    printf("CPU Halted!\n");
}

#include <ctype.h>

int cargarProgramaDesdeArchivo(CPU *cpu, const char *nombreArchivo) {
    FILE *file = fopen(nombreArchivo, "r");
    if (!file) {
        printf("Error: No se pudo abrir el archivo %s\n", nombreArchivo);
        return -1;
    }

    char linea[128];
    int i = 0;

    while (fgets(linea, sizeof(linea), file) && i < MEM_SIZE) {
        char *ptr = linea;

        // Quitar espacios en blanco al inicio
        while (isspace(*ptr)) ptr++;

        // Saltar líneas vacías o comentarios
        if (*ptr == '\0' || *ptr == '\n' || *ptr == ';' || *ptr == '#' || *ptr == '/')
            continue;

        // Quitar comentarios tipo "//"
        char *coment = strstr(ptr, "//");
        if (coment) *coment = '\0';

        // Quitar coma al final
        char *coma = strchr(ptr, ',');
        if (coma) *coma = '\0';

        // Convertir texto a número (hex o decimal)
        uint16_t valor = (uint16_t)strtol(ptr, NULL, 0);
        cpu->mem[i++] = valor;
    }

    fclose(file);
    printf("Se cargaron %d palabras desde %s\n", i, nombreArchivo);
    return i;
}




// PROGRAMA PRINCIPAL
// ==================

/*
1. Crear e inicializar CPU
2. Cargar programa de ejemplo en memoria
3. Inicializar registros para prueba
4. Iniciar bucle de ejecución
*/
int main(int argc, char *argv[])
{
    CPU cpu;
    resetCPU(&cpu);

    if (argc < 2) {
        printf("Uso: %s <archivo_programa>\n", argv[0]);
        return 1;
    }

    // Cargar programa desde archivo pasado como argumento
    if (cargarProgramaDesdeArchivo(&cpu, argv[1]) < 0) {
        return 1;
    }

    cpu.acc = 0;
    cpu.x = 0;

    printf("Starting CPU emulation...\n");
    cpu_loop(&cpu);

    return 0;
}
