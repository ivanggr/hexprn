/* 
	elements.c
	Оперирование с единицами информации
*/
#include <stdlib.h>
#include "elements.h"

/* == Оперируемые единицы информации == */

/* -- константы -- */

/* в составе слова 32 = 2^5 бита, для адресации бита в составе слова необходимо пять разрядов */
static const byte word_erase = 31;	// = 11111b; этой маской и логическим 'И' стираем все, кроме пяти разрядов

/* в составе байта 8 = 2^3 бита, для адресации бита в составе слова необходимо три разряда */
static const byte byte_erase = 7; // = 111b; этой маской и логическим 'И' стираем все, кроме трех разрядов

/* массивы масок для выделения байт в составе слова */
static const word BYTE_SHIFT[] = { 0*BYTE_SIZE_IN_BITS, 1*BYTE_SIZE_IN_BITS, \
	2*BYTE_SIZE_IN_BITS, 3*BYTE_SIZE_IN_BITS }; // на сколько разрядов нужно сдвинуть

static const word BYTE_MASK[] = { UCHAR_MAX << BYTE_SHIFT[0], UCHAR_MAX << BYTE_SHIFT[1], \
	UCHAR_MAX << BYTE_SHIFT[2], UCHAR_MAX << BYTE_SHIFT[3] }; // маска

/* --- запись и чтение битов в/из слова, байта --- */

/* запись бита в слово */
word set_bitw(word w, byte bit_number, bit bit_value)
{
    // у номера игнорируем всё, кроме разрядов, необходимых для адресации всех разрядов слова
    bit_number &= word_erase;
        
    // стираем текущее значение бита
    w &= ~((word)0x01 << bit_number);
        
    // у бита записываемого стираем все разряды, кроме младшего    
    // и записываем новое значение бита        
    w |= (bit_value & 0x01) << bit_number;
        
    return w;
}

/* чтение бита из слова */
bit get_bitw(word w, byte bit_number)
{
    // у номера игнорируем всё, кроме разрядов, необходимых для адресации всех разрядов слова	
    // сдвигаем желаемый разряд в нулевой и обнуляем все прочие разряды
    return (w >> (bit_number & word_erase)) & 0x01;
}

/* запись бита в байт */
byte set_bitb(byte b, byte bit_number, bit bit_value)
{
    // у номера игнорируем всё, кроме разрядов, необходимых для адресации всех разрядов байта
    bit_number &= byte_erase;  
    
    // стираем текущее значение бита
    b &= ~((byte)0x01 << bit_number);
    
    // у бита записываемого стираем все разряды, кроме младшего    
    // и записываем новое значение бита        
    b |= (bit_value & 0x01) << bit_number;
        
    return b;
}

/* чтение бита из байта */
bit get_bitb(byte b, byte bit_number)
{
    // у номера игнорируем всё, кроме разрядов, необходимых для адресации всех разрядов байта
    // сдвигаем желаемый разряд в нулевой и обнуляем все прочие разряды
    return (b >> (bit_number & byte_erase)) & 0x01;
}

/* --- запись и чтение тетрад в составе байта --- */
const byte TETRA_ONE = 0x0F;	// 1111 b
const byte TETRA_SHIFT[] = { 0*4, 1*4 };
const byte TETRA_MASK[] = { TETRA_ONE << TETRA_SHIFT[0], TETRA_ONE << TETRA_SHIFT[1] };

/* запись тетрады в байт */
byte set_tetrab(byte b, size_t tetra_number, byte tetra_value)
// устанавливает тетраду (4 разряда) под номером tetra_number в составе байта b
// возвращает исправленный байт 
{
	// если номер тетрады больше их числа, правит нулевую тетраду
	if (tetra_number >= BYTE_SIZE_IN_TETRAS)
		tetra_number = 0;
	tetra_value &= 0xF;    // убираем все разряды, кроме первых четырёх
	b &= ~TETRA_MASK[tetra_number];    // стираем соответствующую тетраду
	b |= tetra_value << TETRA_SHIFT[tetra_number];
	return b;
}

/* чтение тетрады из байта */
byte get_tetrab(byte b, size_t tetra_number)
// возвращает тетраду (4 разряда) под номером tn в составе байта b в младших разрядах, в старших - нули
// возвращаемое значение: от 0x00 до 0x0F
{
	// если номер тетрады больше их числа, возвращает нулевую тетраду
	if (tetra_number >= BYTE_SIZE_IN_TETRAS)
		tetra_number = 0;
	b &= TETRA_MASK[tetra_number];
	b >>= TETRA_SHIFT[tetra_number];
	return b;
}


/* --- запись и чтение байт в составе слова --- */

/* запись байта в слово */
word set_bytew(word w, size_t byte_number, byte byte_value)
{
	if (byte_number >= WORD_SIZE_IN_BYTES)
		return 0;
	w &= ~BYTE_MASK[byte_number];
	w |= byte_value << BYTE_SHIFT[byte_number];
	return w;
}

/* чтение байта из слова */
byte get_bytew(word w, size_t byte_number)
{
	if (byte_number >= WORD_SIZE_IN_BYTES)
		byte_number = 0;
	w &= BYTE_MASK[byte_number];
	w >>= BYTE_SHIFT[byte_number];
	return (byte) w;
}


/* --- разбиение слова на массив байт, формирование слова из массива байт --- */

/* разбиение слова на массив байт */
int split_word(byte *byte_mas, word srcw, endian_types dest_endian_type)
/* Разбивает слово на отдельные байты. Располагает байты в требуемом порядке в массиве:
     от младших байтов к старшим, если dest_endian_type == LITTLE_ENDIAN;
	 от старших байтов к младшим, если dest_endian_type == BIG_ENDIAN. 
   Возвращает число преобразованных байт.
*/
{
        /* проверка аргумента */
        if (byte_mas == NULL)
            return -1;
    
        /* локальные переменные */       
        int dest_counter, dest_inc, src_counter, byte_count = 0;
       
	/* задание направления расположения байт в массиве-приемнике */
	if (dest_endian_type == LITTLE_ENDIAN)
	{
		dest_counter = 0;
		dest_inc = +1;
	}
	else
	{
		dest_counter = WORD_SIZE_IN_BYTES-1;
		dest_inc = -1;
	}

	/* Выделение байт. В слове последовательно выделяются байты, начиная с нулевого, 
	а потом располагаются в желаемом порядке в массиве */		
	for (src_counter = 0; src_counter < WORD_SIZE_IN_BYTES; src_counter++, dest_counter += dest_inc)
	{
		byte_mas[dest_counter] = (byte) ((srcw & BYTE_MASK[src_counter]) >> BYTE_SHIFT[src_counter]);
		byte_count++;
	}

	return byte_count;
}

/* формирование слова из массива байт */
word form_word(byte *byte_mas, endian_types src_endian_type)
/*	Создает слово из массива байт в общем виде и возвращает его.
	Использует количество байт, равное размеру слова в байтах.
	Слово формируется исходя из заданного порядка байт в src_endian_type:
      нулевой элемент массива - младший байт, если src_endian_type == LITTLE_ENDIAN;
	  нулевой элемент массива - старший байт, если src_endian_type == BIG_ENDIAN.
 */
{
        /* проверка аргумента */	
        if (byte_mas == NULL)
		return 0x00;

	/* локальные переменные */
	int src_counter;
	word dest_word = 0;
	int dest_counter, dest_inc;

	/* задание направления расположения байт в слове-приемнике */
	if (src_endian_type == LITTLE_ENDIAN)
	{
		dest_counter = 0;
		dest_inc = +1;
	}
	else
	{
		dest_counter = WORD_SIZE_IN_BYTES-1;
		dest_inc = -1;
	}

	/* цикл счета байт из массива и размещения их в слове */		
	for (src_counter = 0; src_counter < WORD_SIZE_IN_BYTES; src_counter++, dest_counter += dest_inc)
	{
		dest_word += ((word) byte_mas[src_counter]) << BYTE_SHIFT[dest_counter];
	}

	return dest_word;
}


/* --- быстрое формирование слова из четырех байт, если у нас размер слова равен четырём байтам --- */

/* быстро формирует слово из четырех байт в прямом порядке */
inline static word word_BIG_ENDIAN4(byte b0, byte b1, byte b2, byte b3)
{
	return (((word) b0) << BYTE_SHIFT[3]) + (((word) b1) << BYTE_SHIFT[2]) + (((word) b2) << BYTE_SHIFT[1]) + (((word) b3) << BYTE_SHIFT[0]);	
}

/* быстро формирует слово из четырех байт в обратном порядке */
inline static word word_LITTLE_ENDIAN4(byte b0, byte b1, byte b2, byte b3)
{
	return word_BIG_ENDIAN4(b3, b2, b1, b0);	
}

/* быстро формирует слово из четырех байт в заданном порядке */
word form_word4(byte *byte_mas, endian_types src_endian_type)
{
	if (src_endian_type == LITTLE_ENDIAN)
		return word_LITTLE_ENDIAN4(byte_mas[0], byte_mas[1], byte_mas[2], byte_mas[3]);
	else
		return word_BIG_ENDIAN4(byte_mas[0], byte_mas[1], byte_mas[2], byte_mas[3]);
}

/* --- функции преобразования байт, массивов байт в строки символов --- */

/* преобразует младшую тетраду байта в одну шестнадцатеричную цифру и возвращает её символ */
static char tetra_char[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                               'A', 'B', 'C', 'D', 'E', 'F' };
inline char tetra_hex(byte tetra)
{
	return tetra_char[tetra & 0xF];
}

/* преобразует байт в последовательность шестнадцатеричных цифр и записывает символы в строку s */
int byte_hex(const byte b, char *s, const endian_types endian_type)
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
{
	if (s == NULL)
		return -1;

	/* тетрады проходятся с нулевой тетрады,
	в массив записывается либо с нулевого элемента, либо с последнего */ 

	/* локальные переменные */
	int char_counter = 0;  // число записанных символов
	size_t j = 0;	// счетчик номера тетрады
	int i;          // счетчик массива s
	int delta;      // прирост счетчика массива s

	/* определение элементов у массива */
	if (endian_type == LITTLE_ENDIAN)
	{
		i = 0;
		delta = 1;
	}
	else
	{
		i = ((int) BYTE_SIZE_IN_TETRAS) - 1;
		delta = -1;
	}

	/* цикл преобразования */
	for (; j < BYTE_SIZE_IN_TETRAS; j++, i+= delta)
	{
		s[i] = tetra_hex(get_tetrab(b, j));
		char_counter++;
	}

	return char_counter;
}

/* преобразует байт в последовательность двоичных цифр и записывает символы в строку s */
int byte_bin(const byte b, char *s, const endian_types endian_type)
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
{
	if (s == NULL)
		return -1;

	/* локальные переменные */
	int char_counter = 0;   // число записанных символов
	size_t j = 0;		    // счетчик бита
	int i;                  // счетчик массива s
	int delta;              // прирост счетчика массива s

	/* определение порядка записи в массив s */
	if (endian_type == LITTLE_ENDIAN)
	{
		i = 0;
		delta = 1;
	}
	else
	{
		i = ((int) BYTE_SIZE_IN_BITS) - 1;
		delta = -1;
	}

	/* цикл преобразования */
	for (; j < BYTE_SIZE_IN_BITS; j++, i+= delta)
	{
		s[i] = ((b>>j) & 0x01) == 0 ? '0' : '1';
		char_counter++;
	}

	return char_counter;
}

/* преобразует байт в последовательность двоичных или шестнадцатеричных цифр и записывает символы в строку s */
int byte_base(const byte b, char *s, const endian_types endian_type, const number_base base)
{
	return base == BASE_HEX ? byte_hex(b, s, endian_type) : byte_bin(b, s, endian_type);
}

/* преобразует массив байт в массив двоичных или шестнадцатеричных цифр */
int byte_trans(const byte *bm, char *s, size_t count, const struct trans_mode tm)
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
{
	/* первичная обработка аргументов */
	if (bm == NULL || s == NULL)
		return -1;
	if (count == 0)
		return 0;
	
	/* локальные переменные */
	int result;              // результат преобразования отдельного байта
	int char_counter = 0;    // число записанных символов
	int mas_counter = 0;     // счетчик массива s
	int gap_counter = 0;   // счетчик выведенных символов с последнего разделителя

	size_t byte_counter;     // счетчик байтов
	int byte_inc;            // прирост счетчика массива s
	size_t byte_stop;        // остановка байта

	/* подготовка переменных */
	if (tm.seq_endian == LITTLE_ENDIAN)
	{
		byte_counter = count-1;  // байты обрабатываем с последнего
		byte_inc = -1;           // мы двигаемся назад
		byte_stop = 0;           // останавливаемся на нулевом байте
	}
	else
	{
		byte_counter = 0;     // байты обрабатываем с первого
		byte_inc = +1;        // мы двигаемся вперед
		byte_stop = count-1;  // останавливаемся на последнем байте
	}
	
	/* цикл преобразования */
	/* контроль цикла идет по байтам */
	for (;;)
	{
		/* преобразование отдельного байта */
		result = byte_base(bm[byte_counter], s+mas_counter, tm.byte_endian, tm.base);
		if (result <= 0)
			return char_counter; // если результат не более 0, то возврат
		char_counter += result; // увеличим счетчик выведенных символов
		mas_counter += result;  // увеличим счетчик для символа-разделителя
		gap_counter++;  // увеличим счетчик для символа-разделителя

		/* проверка необходимости установки символа-разделителя */
		if (!(tm.gap == 0 || tm.gap > count))
		{
			if (gap_counter == tm.gap)
			{
				s[mas_counter++] = tm.gap_delim;
				char_counter++;
				gap_counter = 0;
			}
		}

		/* проверка необходимости выхода из цикла */
		if (byte_counter == byte_stop)
			break;
		else
			byte_counter += byte_inc;
	}

	return char_counter;
}

/* преобразует слово w в массив двоичных или шестнадцатеричных цифр */
int word_trans(const word w, char *s, const struct trans_mode tm)
/* параметры и возврат аналогичны функции byte_trans() */
{
	if (s == NULL)
		return -1;
	byte bm[WORD_SIZE_IN_BYTES];
	split_word(bm, w, BIG_ENDIAN);
	return byte_trans(bm, s, WORD_SIZE_IN_BYTES, tm);
}

/* преобразует слово w в массив шестнадцатеричных цифр без пробелов в порядке BIG_ENDIAN */
int word_hex(const word w, char *s)
{
	if (s == NULL)
		return -1;	
	/* параметры преобразования */	
	struct trans_mode tm;
	tm.base = BASE_HEX;
	tm.byte_endian = BIG_ENDIAN;
	tm.seq_endian = BIG_ENDIAN;
	tm.gap = 0;
	tm.gap_delim = ' ';
	return word_trans(w, s, tm);
}