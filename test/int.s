.export _startup,__exit
.forceimport    _irq
.segment    "CODE"
    
_startup:
    PLA
    PHA
    AND #$10
    BEQ L0
    JSR _irq
L0:
    RTI
__exit:
    .byte $FF
