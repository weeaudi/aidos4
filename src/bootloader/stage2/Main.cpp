extern "C" void stage2_main() {
    short int x = 0;

    short int* y = (short int*)0xB8000;

    for(int i = 0; i < (25*80); i++){
        *y = x;
        y++;
    }

    while (true) {
        
    }
}