.extern ad, b
.section ii
cmp r2, r3
jeq %ad
halt

.section iii
.skip 5
ff:
.word b
.word 1
.word 2
.skip 2
gg:
.skip 4

.end
