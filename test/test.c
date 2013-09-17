void _exit(void);
void startup(void);
static char buff[16];
static int n = 0;
void irq(void){
    buff[(n++)&0xf] = *(char *)0x4016;
}

static char getchar(void){
    while(!n);
    return buff[n--];
}

static int memcmp(char *s1,char *s2,int len){
    int i = 0;
    while(i < len - 1){
        if(s1[i] != s2[i]) return s1 - s2;
    }
    return s1 - s2;
}

static char password[8];
typedef void (*fn)(void);
int main(void){
    *(char *)0xFFFE = ((short)startup) & 0xff;
    *(char *)0xFFFF = ((short)startup) >> 8 & 0xff;
    {
        int i = 0;
        while(i < 8){
            password[i++] = getchar();
        }
        if(!memcmp("HELLO123",password,8)){
            while(1);
        }
        _exit();
    }
    return 0;
}
