.section i
a:
.word 27
b:
.skip 3
c:
.word a
d:
.word gg
.section ii
ldr r0, $0
push r3
ad:
mul r1, r0
sub r2, r3
pop pc
not r5
jmp *[r3+ad]
str r4, dg

.global a, b, c

.global ad

.extern dg

.equ gg, 0x5155

.end
