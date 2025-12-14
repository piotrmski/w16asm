loop: ld text       ; Load a character
jmz END             ; If end of string, then terminate
st IO               ; Put charater to terminal

ld loop             ; Increment the character pointer
add const1
st loop

jmp loop            ; Loop to character loading

END: jmp END        ; End of program    

const1: 1
text: "Hello, world!"
IO: .org 0x1fff