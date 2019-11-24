/* 
	elements.h
	Оперирование с единицами информации
*/
#ifndef ELEMENTS_H
#define ELEMENTS_H

#include <stddef.h>
#include <limits.h>

/* == Оперируемые единицы информации == */
/* базовые единицы */
typedef unsigned int word;  // unsigned выбран для битовых сдвигов - заполняется нулями
typedef unsigned char byte; // задание байта, он должен быть не более слова
typedef unsigned char bit;  // бит хранится в младшем разряде, остальные - игнорируются
typedef int endian_types;   // порядок хранения и представления
typedef int number_base;    // база единицы

/* наибольшие значения слова, байта, бита, тетрады */
#define WORD_MAX  UINT_MAX
#define BYTE_MAX  UCHAR_MAX
#define BIT_MAX   1
#define TETRA_MAX 0xF

/* размеры тетрады, байта, слова */
#define TETRA_SIZE_IN_BITS  4
#define BYTE_SIZE_IN_BITS   (CHAR_BIT * sizeof(byte))
#define WORD_SIZE_IN_BITS   (CHAR_BIT * sizeof(word))
#define BYTE_SIZE_IN_TETRAS (BYTE_SIZE_IN_BITS / TETRA_SIZE_IN_BITS + (BYTE_SIZE_IN_BITS % TETRA_SIZE_IN_BITS != 0 ? 1 : 0))
#define WORD_SIZE_IN_BYTES  (WORD_SIZE_IN_BITS / BYTE_SIZE_IN_BITS)

/* порядок хранения и представления */
enum endian_types_v {LITTLE_ENDIAN=0, BIG_ENDIAN=1};
#define WORD_ENDIAN LITTLE_ENDIAN	// тип по умолчанию в словах

/* операции с битами */
#define BIT_SET   1
#define BIT_CLEAR 0

/* представления слова, байта */
enum number_base_v {BASE_HEX = 0, BASE_BIN = 2};

/* структура, которая задает параметры преобразования массива байт */
struct trans_mode
{
	number_base base;         // преобразуется в массив двоичных или шестнадцатеричных цифр
	endian_types seq_endian;  // порядок преобразования группы байт
	endian_types byte_endian; // порядок преобразования байта
	size_t gap;	// число байт, после которого ставится символ-разделитель группы байт
	char gap_delim;	// символ-разделитель группы байт, если ноль или больше количества байт, то не ставится
};

/* запись и чтение битов в составе слова */
word set_bitw(word w, byte bit_number, bit bit_value);
bit get_bitw(word w, byte bit_number);

/* запись и чтение битов в составе байта */
byte set_bitb(byte b, byte bit_number, bit bit_value);
bit get_bitb(byte b, byte bit_number);

/* запись и чтение тетрад в составе байта */
byte set_tetrab(byte b, size_t tetra_number, byte tetra_value);
byte get_tetrab(byte b, size_t tetra_number);

/* запись и чтение байт в составе слова */
word set_bytew(word w, size_t byte_number, byte byte_value);
byte get_bytew(word w, size_t byte_number);

/* разбиение слова на массив байт, формирование слова из массива байт в общем случае */
int split_word(byte *byte_mas, word srcw, endian_types dest_endian_type);
/* Разбивает слово на отдельные байты.
   Располагает байты в требуемом порядке в массиве:
        от младших байтов слова к старшим, если dest_endian_type == LITTLE_ENDIAN (равно нулю);
        от старших байтов слова к младшим, если dest_endian_type == BIG_ENDIAN (не равно нулю). 
   Возвращает число преобразованных байт.
*/
word form_word(byte *byte_mas, endian_types src_endian_type);
/*  Создает слово из массива байт в общем виде и возвращает его.
Использует количество байт, равное размеру слова в байтах.
Слово формируется исходя из заданного порядка байт в src_endian_type:
    нулевой элемент массива - младший байт слова, если src_endian_type == LITTLE_ENDIAN;
    нулевой элемент массива - старший байт слова, если src_endian_type == BIG_ENDIAN.
 *  если отлично от них, то - 
 */

/* быстрое формирование слова из четырех байт */
word form_word4(byte *byte_mas, endian_types src_endian_type);

/* --- Функции преобразования байт и массива байт в массивы символов --- */

/* преобразует младшую тетраду байта в шестнадцатеричную цифру и возвращает её символ */
char tetra_hex(byte tetra);

/* преобразует байт в последовательность шестнадцатеричных цифр и записывает символы в строку s */
int byte_hex(const byte b, char *s, const endian_types endian_type);
/* 
Преобразует отдельный байт в последовательность шестнадцатеричных цифр и записывает её в строку s.
конечный ноль не ставится. Одна тетрада - одна цифра.
Параметры:
	b  -  выводимый байт
	s  -  указатель массива символов для вывода
	endian_type  -  порядок вывода (младшая тетрада идет первой или последней)
Возврат:
	>= 0	число записанных символов
	 < 0	ошибка
*/

/* преобразует байт в последовательность двоичных цифр и записывает символы в строку s */
int byte_bin(const byte b, char *s, const endian_types endian_type);
/* 
Преобразует отдельный байт в последовательность двоичных цифр и записывает её в строку s.
конечный ноль не ставится. Одна тетрада - четыре цифры.
Параметры:
	b  -  выводимый байт
	s  -  указатель массива символов для вывода
	endian_type  -  порядок вывода (младшая тетрада идет первой или последней)
Возврат:
	>= 0	число записанных символов
	 < 0	ошибка
*/

/* преобразует байт в последовательность двоичных или шестнадцатеричных цифр и записывает символы в строку s */
int byte_base(const byte b, char *s, const endian_types endian_type, const number_base base);

/* преобразует массив байт в массив двоичных или шестнадцатеричных цифр */
int byte_trans(const byte *bm, char *s, size_t count, const struct trans_mode tm);
/* 
Преобразует массив байт в последовательность цифр и записывает её в строку s.
Параметры:
	bm  -  массив байт
	s   -  строка символов для записи
	count - количество байт для преобразования
	tm  -  формат преобразования
Возврат:
	>= 0	число записанных символов
	 < 0	ошибка
*/

/* преобразует слово w в массив двоичных или шестнадцатеричных цифр */
int word_trans(const word w, char *s, const struct trans_mode tm);
/* параметры и возврат аналогичны функции byte_trans() */
		
/* преобразует слово w в массив шестнадцатеричных цифр без пробелов в порядке BIG_ENDIAN */
int word_hex(const word w, char *s);

#endif //ELEMENTS_H