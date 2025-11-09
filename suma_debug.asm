LD ACC,[10]    // 0
ADD ACC,[11]   // 1
ADD ACC,[12]   // 2
ST ACC,[13]    // 3
HALT          // 4

; el programa ocupa 5 líneas (direcciones 0–4),
; así que los datos empiezan justo en mem[5]
; relleno con 0 para ver mas claro en el debug la operacion
mem[5] = 0
mem[6] = 0
mem[7] = 0
mem[8] = 0
mem[9] = 0

mem[10] = 1
mem[11] = 2
mem[12] = 4

