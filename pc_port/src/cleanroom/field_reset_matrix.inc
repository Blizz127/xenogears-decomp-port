void func_80070594(MATRIX *dest) {
    SVECTOR angles = {0};

    RotMatrix(&angles, dest);
    dest->t[0] = 0;
    dest->t[1] = 0;
    dest->t[2] = 0;
}
