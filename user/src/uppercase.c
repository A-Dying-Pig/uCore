#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(){
    char str[200], temp;
    int len = 10;
    for(int i = 0; i < len; i++){
        temp = getchar();
        str[i] = temp;
    };
    for (int i = 0; i < len; i ++){
        if (str[i] < 123 && str[i] > 96)
            str[i] = str[i] - 32;
    }
    puts(str);
    return 0;
}