LD ACC,[5]    // 0
ADD ACC,[6]   // 1
ADD ACC,[7]   // 2
ST ACC,[8]    // 3
HALT          // 4

; el programa ocupa 5 líneas (direcciones 0–4),
; así que los datos empiezan justo en mem[5]
mem[5] = 1
mem[6] = 2
mem[7] = 4

