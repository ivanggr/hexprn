/* Пример работы hex_prn */
#include "hexprn.h"

/* Возвращает массив байт длиной byte_count, заполненный 
 заглавными буквами английского алфавита */
byte *English_Array(size_t byte_count)
{
    const int ENGLIGH_LETTERS = 26;
    int i;
	
    // проверка аргументов
    if (byte_count == 0)
	return NULL;
    
    // запрос памяти
    byte *bm = (byte *) calloc(byte_count, sizeof(byte));
    if (bm == NULL)
	return NULL;

    // заполнение массива
    for (i = 0; i < byte_count; i++) 
	bm[i] = i % ENGLIGH_LETTERS + (byte) 'A';

    return bm;
}

int main() 
{
    /* создаём массив символов */
    int j;
    const int BYTE_COUNT = 16*20 + 10;
    byte *bm = English_Array(BYTE_COUNT);
    if (bm == NULL)
        return -1;
    printf("\nThere is an array full of English letters:\n\n");
    for (j = 0; j < BYTE_COUNT; j++)
        putchar((int) bm[j]);
    putchar('\n');
    
    /* печать стандартным средством */
    printf("\nWe could look at it as a hex:\n\n");
    struct Trans_Result tr;
    tr = hexprn(bm, BYTE_COUNT);
    printf("----------\n");
    print_tr(&tr); printf("----------\n");
    
    /* печать в другом формате */
    char *s;
    struct Trans_Format tf; 
    tf = ret_default_tf();
    tf.hex_block_length = 4;
    tf.ascii_block_delimeter = '|';
    tf.ascii_block_length = 4;    
    tr = calc_tr_result(BYTE_COUNT, 0, &tf, "\n");    
    s = (char *) calloc(tr.char_count+1, sizeof(char)); s[tr.char_count] = '\0';
    tr = shexprn_formatted(s, bm, BYTE_COUNT, 0xFF14, &tf, "\n", tr);
    printf("\nAlso we could look at it in a different way:\n\n");  
    printf("%s", s);
    printf("----------\n");
    print_tr(&tr); printf("----------\n");   
    free(s);  
    
    free(bm);
    return 0;
}

