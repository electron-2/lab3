// linker memmap places these symbols at start/end of bss
extern int __bss_start__, __bss_end__;

extern void main();

// The C function _cstart is called from the assembly in start.s
// _cstart zeroes out the BSS section and then calls main.
// After main() completes, turns on the green ACT LED as
// a sign of successful execution.
void _cstart() {
    int *bss = &__bss_start__;
    int *bss_end = &__bss_end__;

    while (bss < bss_end) {
        *bss++ = 0;
    }

    main();
}
