# ensamblador_final_bin.py
import re
import sys
import os

# ==============================
# TABLA DE OPCODES (según mainNS.c)
# ==============================
OPCODES = {
    "ST": 0,
    "LD": 1,
    "ADD": 2,
    "BR": 3,
    "BZ": 4,
    "CLR": 5,
    "DEC": 6,
    "HALT": 7
}

def ensamblar_linea(linea):
    linea = linea.strip().upper()

    # Ignorar líneas vacías o comentarios
    if not linea or linea.startswith(";"):
        return None

    # Instrucción HALT
    if linea.startswith("HALT"):
        return "0xE00, // HALT"

    # Coincidir instrucciones tipo: INSTR REG,[ADDR]
    match = re.match(r'(\w+)\s+(\w+),\s*\[(\d+)\]', linea)
    if not match:
        return None  # Puede ser un dato (mem[...]) o comentario

    instr, reg, addr = match.groups()
    addr = int(addr)
    if instr not in OPCODES:
        raise ValueError(f"Instrucción desconocida: {instr}")

    opcode = OPCODES[instr]
    reg_bit = 1 if reg == "ACC" else 0
    dirm = 0  # modo directo

    valor = (opcode << 9) | (reg_bit << 8) | (dirm << 6) | (addr & 0x3F)
    return f"0x{valor:03X}, // {instr} {reg},[{addr}]"

def parsear_datos(linea):
    linea = linea.strip()
    # Coincidir: mem[xx] = valor
    match = re.match(r'MEM\[(\d+)\]\s*=\s*(0X[0-9A-F]+|\d+)', linea, re.IGNORECASE)
    if match:
        direccion = int(match.group(1))
        valor_str = match.group(2)
        valor = int(valor_str, 0)  # interpreta decimal o hexadecimal
        return direccion, valor
    return None

def ensamblar_archivo(nombre):
    instrucciones = []
    datos = []

    with open(nombre, "r") as f:
        for linea in f:
            # Intentar dato primero
            dato = parsear_datos(linea)
            if dato:
                datos.append(dato)
                continue

            # Intentar instrucción
            cod = ensamblar_linea(linea)
            if cod:
                instrucciones.append(cod)

    # Ordenar datos por dirección
    datos.sort(key=lambda x: x[0])

    # Formatear salida
    salida = "\n".join(instrucciones) + "\n\n"
    for dir, val in datos:
        salida += f"{val}, // mem[{dir}]\n"

    return salida.strip()

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("❌ Uso: python ensamblador_final_bin.py <archivo.asm>")
        sys.exit(1)

    archivo_asm = sys.argv[1]
    if not os.path.exists(archivo_asm):
        print(f"⚠️ El archivo '{archivo_asm}' no existe.")
        sys.exit(1)

    # Crear nombre de salida .bin
    archivo_bin = os.path.splitext(archivo_asm)[0] + ".bin"

    print(f"📘 Ensamblando {archivo_asm} ...")

    try:
        resultado = ensamblar_archivo(archivo_asm)

        # Guardar salida
        with open(archivo_bin, "w") as out:
            out.write(resultado + "\n")

        print(f"✅ Ensamblado exitoso. Archivo generado: {archivo_bin}")

    except Exception as e:
        print(f"⚠️ Error: {e}")
