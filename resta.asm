; Programa: resta funcional (mem[12] = 8 - 2 = 6)

LD ACC,[10]    // ACC = 8
ADD ACC,[11]   // ACC = ACC + (-2) = 6
ST ACC,[12]    // Guardar resultado
HALT           // Detener CPU

; Relleno para alinear memoria
mem[4] = 0
mem[5] = 0
mem[6] = 0
mem[7] = 0
mem[8] = 0
mem[9] = 0

; Datos reales
mem[10] = 8        ; Minuendo
mem[11] = 0xFFFE   ; Complemento a dos de 2 (-2 en uint16_t)
mem[12] = 0        ; Espacio para el resultado
