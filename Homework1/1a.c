int oneA() {
    // 1a
    unsigned char count = 4;
    unsigned int inVal, out, kk = 4096, mm = 6;

    while (kk > 0) {
        inVal = (mm+kk)/kk;
        mm /= 2;
        kk = kk << 1;
        if (inVal == 0) {
            out = 1;
        }
    }

    while (count > 0) {
        count = count + 4;
    }

    return out;
}
