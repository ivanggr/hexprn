/* Код проекта hex_prn */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "elements.h"
#include "hexprn.h"


enum CellType {cell_empty, cell_byte};
enum ASCIIType {value_hex, value_ascii};

/* ASCII коды управляющих символов */
#define CHAR_NUL 0x00  // Null Character
#define CHAR_LF  0x0A  // Line Feed
#define CHAR_CR  0x0D  // Carriage Return
#define CHAR_US  0x1F  // Unit Separator
#define CHAR_DEL 0x7F  // Delete

/* Структура, определяющая одну ячейку */
static struct Cell
{
	int type;  // тип ячейки
	word address;  // адрес байта
	byte value;    // значение байта
	char hex[BYTE_SIZE_IN_TETRAS];  // значение ячейки в hex [BYTE_SIZE_IN_TETRAS]
	char ascii;    // отображаемый символ ячейки ASCII
};

/* Структура, определяющая адресную строку */
static struct AddressString
{
	struct Cell cells[0x10];    // массив ячеек
	word address_start;  // адрес, соответствующий нулевому элементу массива байт
	byte *byte_array;    // массив байт
	size_t byte_count;   // число байт в адресной строке для преобразования, не более 0x10
	size_t bytes_start;  // номер ячейки, с какой записываются байты
	size_t bytes_finish; // номер ячейки, на какой заканчивается заканчивается запись байтов
};


/* печать результатов преобразования tr в файл fp */
int fprint_tr(FILE *fp, struct Trans_Result *tr)
{
	if (tr == NULL || fp == NULL)
		return 0;
	return fprintf(fp, "Transform result:\nbyte count: %d.\nchar count: %d.\nstring count: %d.\nsingle string length inc. add.str: %d.\nadd.string length: %d.\n",
		tr->byte_count, tr->char_count, tr->str_count, tr->single_length, tr->add_length);
}

/* печать результатов преобразования tr в стандартный вывод stdout */
inline int print_tr(struct Trans_Result *tr)
{
	return fprint_tr(stdout, tr);
}

/* возврат формата вывода по умолчанию */
struct Trans_Format ret_default_tf()
{
	struct Trans_Format tf;

	/* параметры адреса */
	tf.prn_address = 1; // истина, печать адрес

	/* параметры пустых ячеек */
	tf.empty_value = 0x00;
	tf.empty_hex = '*';
	tf.empty_ascii = '.';

	/* параметры вывода шестнадцатеричных значений ячеек */
	tf.hex_char_delimeter = ' ';
	tf.hex_block_delimeter = '|';
	tf.hex_block_length = 0x8;

	/* параметры вывода ascii значений ячеек */
	tf.ascii_char_delimeter = '\0';
	tf.ascii_block_delimeter = '\0';
	tf.ascii_block_length = 0;
	tf.non_print_char = '.';

	/* возврат */
	return tf;
}

/* Подсчет и возврат количества символов для вывода одной адресной строки в формате tf */
size_t calc_chars_tf(struct Trans_Format *tf)
{
	size_t l = 0;

	/* проверка аргумента */
	if (tf == NULL)
		return 0;

	/* подсчет адреса */
	if (tf->prn_address)
	{
		l += WORD_SIZE_IN_BYTES * BYTE_SIZE_IN_TETRAS; 
		// учет символов ':' и ' ' после адреса
		l += 2;
	}

	/* подсчет вывода шестнадцатеричных значений ячеек */
	l += 0x10 * BYTE_SIZE_IN_TETRAS;  // непосредственные значения

	// если есть разделитель между ячейками
	if (tf->hex_char_delimeter != '\0' && tf->hex_char_delimeter != CHAR_DEL)
		l += 0x10;

	// если есть разделитель между группами
	if (tf->hex_block_delimeter != 0 && tf->hex_block_delimeter != '\0' && 
		tf->hex_block_delimeter != CHAR_DEL)
	{
		l += 0x10 / tf->hex_block_length;
		if (tf->hex_char_delimeter != '\0' && tf->hex_char_delimeter != CHAR_DEL)
			l += 0x10 / tf->hex_block_length; // если есть просто разделитель
	}
	
	/* подсчет вывода ascii значений ячеек */
	l += 0x10;  // непосредственные ascii значения

	// если есть разделитель между ячейками
	if (tf->ascii_char_delimeter != '\0' && tf->ascii_char_delimeter != CHAR_DEL)
		l += 0x10;

	// если есть разделитель между группами
	if (tf->ascii_block_delimeter != 0 && tf->ascii_block_delimeter != '\0' &&
		tf->ascii_block_delimeter != CHAR_DEL)
	{
		l += 0x10 / tf->ascii_block_length;
		if (tf->ascii_char_delimeter != '\0' && tf->ascii_char_delimeter != CHAR_DEL)
			l += 0x10 / tf->ascii_block_length; // если есть просто разделитель
	}

	return l;
}

/* проверка: печатаемый/непечатаемый символ */
static inline int non_print_char(char c)
{
	return (c <= CHAR_US || c == CHAR_DEL) ? 1 : 0;
}

/* инициализация структуры адресной строки addr_str */
static struct AddressString *init_addr_str(struct AddressString *addr_str,
	byte *byte_array, size_t byte_count, word address_start)
{
	/* проверка аргументов */
	if (addr_str == NULL || byte_array == NULL)
		return NULL;
	
	/* массив байт */
	addr_str->byte_array = byte_array;

	/* начальный адрес нулевого элемента массива */
	addr_str->address_start = address_start;
	
	/* установка номера начальной байтовой ячейки */
	addr_str->bytes_start = addr_str->address_start & 0xF;

	/* число байт в строке для преобразования не более 0x10 */
	addr_str->byte_count = byte_count;
	if (addr_str->byte_count > 0x10)
		addr_str->byte_count = 0x10;

	/* установка номера конечной байтовой ячейки */
	/* если число равно байт для вывода равно нулю */
	if (addr_str->byte_count == 0)
	{
		addr_str->bytes_finish = addr_str->bytes_start;
	}
	else
	{
		addr_str->bytes_finish = addr_str->bytes_start + addr_str->byte_count - 1;
		// конечная байтовая ячейка имеет номер не более 0xF
		if (addr_str->bytes_finish > 0xF)
		{
			addr_str->bytes_finish = 0xF;
			addr_str->byte_count = addr_str->bytes_finish - addr_str->bytes_start + 1; // правка числа байт
		}
	}
	return addr_str;
}

/* инициализация массива ячеек по заданным значениям и инициализированной структуре адресной строки */
static struct AddressString *init_cells(struct AddressString *addr_str,
	byte empty_value, char empty_hex, char empty_ascii,
	char non_print_ch)
/* Инициализирует массив ячеек адресной строки addr_str. Непечатаемые символы заменяются на печатаемые.
Параметры:
	addr_str     - адресная строка 
	empty_value  - что записывается в значение пустых ячеек
	empty_hex    - какой символ отображает шестнадцатеричное значение пустых ячеек
	empty_ascii  - какой символ отображает ascii значение пустых ячеек
	non_print_ch - какой символ показывает шестнадцатеричное значение непечатаемых символов
Возврат:
	!= NULL  инициализация успешна
	== NULL  инициализация неуспешна
*/
{
	/* проверка аргументов */
	if (addr_str->byte_array == NULL && addr_str->byte_count != 0)
		return NULL;

	/* непечатаемые символы заменяются на печатаемые */
	if (non_print_char(empty_hex))
		empty_hex = ' ';
	if (non_print_char(empty_ascii))
		empty_ascii = ' ';
	if (non_print_char(non_print_ch))
		non_print_ch = ' ';

	/* инициализация ячеек */
	addr_str->cells[0].address = addr_str->address_start & ~0xF;
	size_t j = 0;
	size_t i; 
	
	if (addr_str->byte_count != 0) 
	/* если число байт не равно нулю, инициализация первых двух групп */
	{
		/* инициализация пустых ячеек #1 */
		for (; j < addr_str->bytes_start; j++)
		{
			addr_str->cells[j].type = cell_empty;
			addr_str->cells[j].address = addr_str->cells[0].address + j;		
			addr_str->cells[j].value = empty_value;
			for (i = 0; i < BYTE_SIZE_IN_TETRAS; i++)
				addr_str->cells[j].hex[i] = empty_hex;
			addr_str->cells[j].ascii = empty_ascii;
		}

		/* инициализация байт */
		size_t byte_counter = 0;
		for (; j <= addr_str->bytes_finish; j++)
		{
			addr_str->cells[j].type = cell_byte;
			addr_str->cells[j].address = addr_str->cells[0].address + j;		
			addr_str->cells[j].value = addr_str->byte_array[byte_counter];

			// перевод в шестнадцатеричный вид одного байта
			if (byte_hex(addr_str->cells[j].value, addr_str->cells[j].hex, BIG_ENDIAN) < 0)
				return NULL;

			// установка неотображаемого символа
			if (addr_str->byte_array[byte_counter] <= (byte) CHAR_US   || 
				addr_str->byte_array[byte_counter] == (byte) CHAR_DEL  || 
				addr_str->byte_array[byte_counter] >  (byte) CHAR_MAX)
			{
				addr_str->cells[j].ascii = non_print_ch;
			}
			else
			// установка отображаемого символа
			{
				addr_str->cells[j].ascii = (char) addr_str->byte_array[byte_counter];
			}
			byte_counter++;
		}
	}

	/* инициализация пустых ячеек #2 */
	for (; j <= 0xF; j++)
	{
		addr_str->cells[j].type = cell_empty;
		addr_str->cells[j].address = addr_str->cells[0].address + j;		
		addr_str->cells[j].value = empty_value;
		for (i = 0; i < BYTE_SIZE_IN_TETRAS; i++)
			addr_str->cells[j].hex[i] = empty_hex;
		addr_str->cells[j].ascii = empty_ascii;
	}

	return addr_str;
}

/* Выводит в строку s адрес строки addr_str с обнулением младшей тетрады. Возвращает число символов */
static int sprn_address(char *s, struct AddressString *addr_str)
{
	/* проверка аргументов */
	if (addr_str == NULL)
		return -1;
	int r; // результат вызова функции

	/* преобразование адреса */
	r = word_hex(addr_str->address_start & ~0xF, s); if (r < 0) return r;

	/* добавка символов */
	s[r++] = ':';
	s[r++] = ' ';
	return r;
}

/* Выводит значения ячеек (в шестнадцатеричном или ascii виде) адресной строки в строку s. 
Возвращает число символов */
static int sprn_values(char *s, struct AddressString *addr_str, int type,
	char ch_delim, char bl_delim, size_t bl_len)
/* Выводит значения ячеек адресной строки в строку s.
Параметры:
	s         - строка символов для записи
	addr_str  - строка 
	type      - тип для вывода (шестнадцатеричный или ascii)
	ch_delim  - разделитель между выведенными элементами, если '\0' или CHAR_DEL, то не ставится
	bl_delim  - разделитель между группами элементов, если '\0' или CHAR_DEL, то не ставится
	bl_len    - длина группы элементов, между которыми ставится символ '|', если 0, то не ставится
Возврат:
	>= 0	число записанных символов
	 < 0	ошибка
*/
{
	/* проверка аргументов */
	if (s == NULL || addr_str == NULL)
		return -1;

	/* настройка локальных переменных */
	size_t cell_counter;  // счетчик ячеек
	int ch_counter = 0;   // счетчик выведенных символов
	size_t bl_count = 0;  // счетчик для группы
	size_t j;             // счётчик вывода символов

	/* вывод значений ячеек */
	for (cell_counter = 0; cell_counter <= 0xF; cell_counter++)
	{
		// выводим или в шестнадцатеричном или ascii виде
		if (type == value_hex)
		{
			for (j = 0; j < BYTE_SIZE_IN_TETRAS; j++)
				s[ch_counter++] = addr_str->cells[cell_counter].hex[j];
		}
		else
		{
			s[ch_counter++] = addr_str->cells[cell_counter].ascii;
		}

		/* постановка разделителя между ячейками */
		if (ch_delim != '\0' && ch_delim != CHAR_DEL)
			s[ch_counter++] = ch_delim; 

		/* постановка разделителя между группами */
		if (++bl_count == bl_len)
		{
			if (bl_delim != '\0' && bl_delim != CHAR_DEL)
				s[ch_counter++] = bl_delim;
			bl_count = 0;
			/* постановка разделителя между ячейками после разделителя между группами */
			if (ch_delim != '\0' && ch_delim != CHAR_DEL)
				s[ch_counter++] = ch_delim; 
		}
	}
		
	/* возврат числа выведенных символов */
	return ch_counter;
}

/* Вывод строки в шестнадцатеричном виде */
static inline int sprn_values_hex(char *s, struct AddressString *addr_str, 
						   char ch_delim, char bl_delim, size_t bl_len)
{
	return sprn_values(s, addr_str, value_hex, ch_delim, bl_delim, bl_len);
}

/* Вывод в ascii виде */
static inline int sprn_values_ascii(char *s, struct AddressString *addr_str, 
						   char ch_delim, char bl_delim, size_t bl_len)
{
	return sprn_values(s, addr_str, value_ascii, ch_delim, bl_delim, bl_len);
}

/* Преобразование массива байт в массив символов одной адресной строки без добавочной строки */
static struct Trans_Result shexprn_str(char *s, byte *byte_array, size_t byte_count, 
	word address_start, struct Trans_Format *tf)
/* Преобразует byte_count байт из массива байт byte_array в одну адресную строку и 
записывает её в символьную строку s. Нулевому элементу массива byte_array соответствует адрес address_start.
Автоматически определяет и ограничивает число преобразуемых байт. Преобразование согласно формату tf.
Преобразует байты от начального адреса, но не далее, чем вплоть до WORD_MAX.
Параметры:
	s              - символьная строка для записи
	byte_array     - массив исходных байт
	byte_count     - наибольшее число байт для преобразования, если больше 0xF, то сокращается до 0xF,
	               ноль допускается
	address_start  - адрес (смещение) нулевого элемента массива byte_array
	tf             - формат преобразования
Возвращает структуру:
	Trans_Result.char_count = Trans_Result.single_length:
	    < 0   ошибка до непосредственного преобразования
	    > 0   число символов, записанное в строку s:
	        != длина_адресной_строки - ошибка при преобразовании
	        == длина_адресной_строки - корректный результат
	Trans_Result.byte_count:
	    < 0   ошибка
	    >= 0  число преобразованных байт
	Trans_Result.str_count:
	    != 1  есть какая-то ошибка
	    == 1  преобразована одна строка
*/
{
	int r;
	 // предполагается, что функция будет вызываться много раз
	static struct Trans_Result tr;
	tr.byte_count = -1;
	tr.char_count = -1;
	tr.str_count = -1;
	// структура имеет большой размер,
   	// и предполагается, что эта функция будет вызываться много раз,
	// поэтому пусть будет структура статической
	static struct AddressString addr_str;
	
	/* Проверка аргументов */
	if (s == NULL || byte_array == NULL || tf == NULL)
		return tr;

	/* Инициализации структуры, ячеек */
	if (!init_addr_str(&addr_str, byte_array, byte_count, address_start))
		return tr;
	if (!init_cells(&addr_str, tf->empty_value, tf->empty_hex, tf->empty_ascii, tf->non_print_char))
		return tr;
	
	/* Установка предварительных значений */
	tr.byte_count = 0;
	tr.char_count = 0;
	tr.str_count = 0;

	/* Преобразование адреса */
	if (tf->prn_address)
		tr.char_count = sprn_address(s, &addr_str);
        if (tr.char_count < 0)
            return tr;

	/* Преобразование шестнадцатеричных значений */
	r = sprn_values_hex(s + tr.char_count, &addr_str, tf->hex_char_delimeter, 
		tf->hex_block_delimeter, tf->hex_block_length);
	if (r < 0)
		return tr;
	else
		tr.char_count += r;

	/* Преобразование ascii значений */
	r = sprn_values_ascii(s + tr.char_count, &addr_str, tf->ascii_char_delimeter, 
		tf->ascii_block_delimeter, tf->ascii_block_length);
	if (r < 0)
            return tr;
	else
            tr.char_count += r;

	/* Установка числа байт */
	tr.byte_count = addr_str.byte_count;
	tr.single_length = (size_t) tr.char_count;
        tr.str_count = 1; // главный признак успешности преобразования - что преобразована одна строка

	/* Возврат */
	return tr;
}

/* Проверяет ограничение числа байт для преобразования count 
по перекрытию наибольшего доступного адреса WORD_MAX.
Должно выполняться неравенство:
	max_count <= WORD_MAX + 1 - address
Наибольшее количество ячеек, если начальный адрес равен нулю, тогда возможно ячеек WORD_MAX + 1.
Во всех других случаях число байт <= WORD_MAX.
*/
size_t hex_max_count(size_t count, word address)
{
	size_t max_count; 
	word word_count;
	// случай, если размерность size_t больше, чем у word
	if (SIZE_MAX > WORD_MAX)
	{
		max_count = WORD_MAX + 1 - address;
		return max_count < count ? max_count : count;
	}
	// случай, если размерность size_t меньше или равна word
	else
	{
		word_count = address == 0 ? WORD_MAX : WORD_MAX + 1 - address;
		return word_count < (word) count ? (size_t) word_count : count;		
	}	
}

/* Определение, сколько адресных строк требуется для преобразования массива байт. 
Преобразование байт в общем случае может идти с перехлестом.
Перед вызовом этой функции необходимо проверить число байт в hex_max_count(). */
size_t hex_addr_str(size_t count, word address)
{
	// сначала граничные проверки

	// сделаем проверку количества на ноль, чтобы избавиться от ошибок, и на единицу для быстроты
	if (count == 0 || count == 1)
		return 1;

	// проверка предельного значения count >= WORD_MAX + 1
	if (SIZE_MAX > WORD_MAX)
	{
		if (count >= (WORD_MAX + 1))
			return (WORD_MAX >> 4) + 1; // это предел количества строк для преобразования
	}

	/* Порядок вычисления количества строк:
	Начальный адрес задан. Конечный адрес равен начальному плюс количество байт минус единица.
	Получаем порядковые номера адресных строк (отрезков) по 16 байт путем сдвига адресов на четыре разряда влево.
	Так как word беззнаковый, то при любом направлении сдвига (<< и >>) разряды заполняются нулями.
	Затем из номера конечного вычитаем начальный и к результату прибавляем единицу.
	К примеру, у нас задано число байт для преобразования 182 (0xB6) и начальный адрес нулевого байта 0x194: 
		номер начального отрезка 0x194 -> 0x19
		номер конечного отрезка 0x194 + 0xB6 - 0x01 = 0x249 -> 0x24
		число строк 0x24 - 0x19 + 0x01 = 0x0C = 12, то есть двенадцать
	*/
	word q = ((address + count - 0x01) >> 4) - (address >> 4) + 0x01;

	// проверка на SIZE_MAX избыточна, но пусть будет
	return SIZE_MAX < q ? SIZE_MAX : q;
}

/* Подсчитывает и возвращает предполагаемый результат преобразования массива байт
byte_array длиной count в последовательность адресных строк s.
Внимание! Значения полей возвращаемой структуры ограничены INT_MAX, проверка на переполнение не производится.  */
struct Trans_Result calc_tr_result(size_t byte_count, word address_start, struct Trans_Format *tf,\
	char *insert_str)
/* Подсчитывает и возвращает предполагаемый результат преобразования массива байт.
Параметры:
	byte_count     - наибольшее число байт для преобразования, ограничивается hex_max_count()
	address_start  - адрес (смещение) нулевого элемента массива byte_array
	tf             - формат преобразования
	insert_str     - строка, которая будет вставляться после каждой преобразованной адресной строки,
	                 допустим "\r\n" или "\n", если NULL, то без вставки
Возвращает структуру:
	Trans_Result.char_count:
	     < 0  ошибка
	     > 0  необходимое число символов, для записи в строку s с учетом строки insert_str
	Trans_Result.byte_count:
	     < 0  ошибка
	    >= 0  число преобразованных байт, ограничивается hex_max_count()
	Trans_Result.str_count:
		<= 0  ошибка
	   	 > 0  предполагаемое число строк для преобразования
	Trans_Result.single_length
		== 0  ошибка
	   	 > 0  предполагаемая длина одной преобразованной строки с учетом длины добавочной строки
	Внимание! Значения полей возвращаемой структуры ограничены INT_MAX, проверка на переполнение не производится.
*/
{
	// наш возврат
	struct Trans_Result tr; 
        tr.byte_count = -1; tr.char_count = -1; tr.str_count = -1; tr.single_length = 0;

	// проверка аргументов
	if (tf == NULL)
		return tr;

	// длина добавочной строки
	size_t sd = strlen(insert_str);
	if (sd > INT_MAX)
	{
		tr.add_length = 0;
		return tr;
	}
	else
		tr.add_length = (int) sd;

	// число символов для преобразования одной адресной строки
	tr.single_length = calc_chars_tf(tf);
	if (tr.single_length == 0)
		return tr;

	// число строк для преобразования
	tr.str_count = hex_addr_str(byte_count, address_start);
	if (tr.str_count == 0)
		return tr;

	// подсчет количеств
	// учет ограничения количества байт сверху по адресу
	tr.byte_count = hex_max_count(byte_count, address_start);

	// учет длины добавочной строки
	tr.single_length += tr.add_length;

	// учет того, что добавочная строка добавляется и в последнюю строку
	tr.char_count = tr.single_length * tr.str_count;

	return tr;
}

/* Преобразование массива байт длиной byte_count в последовательность адресных строк */
struct Trans_Result shexprn_formatted(char *s, byte *byte_array, size_t byte_count, \
	word address_start, struct Trans_Format *tf, char *insert_str, struct Trans_Result before_tr)
/* Преобразование массива байт длиной byte_count в последовательность адресных строк и записывает их в s.
Параметры:
	s              - символьная строка для записи
	byte_array     - массив исходных байт
	byte_count     - наибольшее число байт для преобразования, ограничивается hex_max_count()
	address_start  - адрес (смещение) нулевого элемента массива byte_array
	tf             - формат преобразования
	insert_str     - строка, которая будет вставляться после каждой преобразованной адресной строки,
	                 допустим "\r\n" или "\n", если NULL, то без вставки
	before_tr      - предварительный результат всего преобразования согласно возврату calc_tr_result()
Возвращает структуру:
	Trans_Result.char_count:
	     < 0  ошибка
	     > 0  число записанных символов
	Trans_Result.byte_count:
	     < 0  ошибка
	    >= 0  число преобразованных байт, не более результата hex_max_count()
	Trans_Result.str_count:
		<= 0  ошибка
	   	 > 0  число преобразованныз строк
	Trans_Result.single_length
		== 0  ошибка
	   	 > 0  длина одной преобразованной строки с учетом длины добавочной строки
	Внимание! Значения полей возвращаемой структуры ограничены INT_MAX, проверка на переполнение не производится.
*/
{
	// результат одного преобразования
	struct Trans_Result tr; 
        tr.byte_count = -1; tr.char_count = -1; tr.str_count = -1; tr.single_length = 0;

	// проверка аргументов
	if (s == NULL || byte_array == NULL || tf == NULL)
		return tr;
	if (before_tr.byte_count < 0 || before_tr.char_count <= 0 || before_tr.single_length <= 0 || before_tr.str_count <= 0)
		return before_tr;

	struct Trans_Result cumul_tr;   // накопленный результат

	/* ограничение числа байт */
	byte_count = hex_addr_str(byte_count, address_start);
	
	/* инициализация накопленного результата */
	cumul_tr.add_length = before_tr.add_length;
	cumul_tr.single_length = before_tr.single_length;
	cumul_tr.byte_count = 0;
	cumul_tr.char_count = 0;
	cumul_tr.str_count = 0;

	/* цикл преобразования */
	int j, i;
	for (j = 0; j < before_tr.str_count; j++)
	{
		/* преобразование одной строки */
		tr = shexprn_str(\
			s + cumul_tr.char_count,            // сдвиг строки символов на число преобразованных символов
			byte_array + cumul_tr.byte_count,   // сдвиг массива байт на число преобразованных байт
			before_tr.byte_count - cumul_tr.byte_count,  // в количестве байт учет количества преобразованных байт
			address_start + cumul_tr.byte_count, // в адресе учет количества преобразованных байт
			tf);
		
		/* проверка результатов */
		if (tr.byte_count < 0 || tr.char_count <= 0 || tr.str_count != 1)
			return cumul_tr;
		cumul_tr.byte_count += tr.byte_count;
		cumul_tr.char_count += tr.char_count;
		cumul_tr.str_count  += tr.str_count;

		/* добавка дополнительной строки, если она есть */
		if (before_tr.add_length != 0)
		{
			for (i = 0; i < before_tr.add_length; i++)
				s[cumul_tr.char_count++] = insert_str[i];
		}
	}

	return cumul_tr;
}

/* Преобразование массива байт длиной byte_count в последовательность адресных строк 
с форматом по-умолчанию. Выделяет необходимую память для строки *s.
Перед использованием необходимо выделить память s = (char **) malloc(sizof(char **));
Когда строка не нужна, требуется освободить память в таком порядке: free(*s); free(s); */
struct Trans_Result shexprn(char **s, byte *byte_array, size_t byte_count, word address_start)
/* Преобразование массива байт длиной byte_count в последовательность адресных строк и записывает их в s.
Параметры:
	s              - указатель на указатель, куда будет записываться строка
	byte_array     - массив исходных байт
	byte_count     - наибольшее число байт для преобразования, ограничивается hex_max_count()
	address_start  - адрес (смещение) нулевого элемента массива byte_array
Возвращает структуру Trans_Result. 	
*/
{
	struct Trans_Result tr, prtr; tr.byte_count = -1; tr.char_count = -1; tr.str_count = -1; tr.single_length = 0;

	/* проверка аргументов */
	if (s == NULL || byte_array == NULL)
		return tr;

	/* задание формата преобразования по-умолчанию */
	struct Trans_Format tf = ret_default_tf();

	/* вычисление формата */
	tr = calc_tr_result(byte_count, address_start, &tf, "\n");
	if (tr.char_count < 0)
		return tr;

	/* подготовка строки для записи */
	*s = (char *) calloc(tr.char_count+1, sizeof(char));
	if (*s == NULL)
	{
		tr.char_count = -1;
		return tr;
	}
	(*s)[tr.char_count] = '\0';

	/* результат преобразования */
	prtr = shexprn_formatted(*s, byte_array, byte_count, address_start, &tf, "\n", tr);
	// если были ошибки при преобразовании
	if (prtr.char_count <= 0)
	{
		free(*s);
		*s = NULL;
		return prtr;
	}
	// если потратили меньше символов, чем требовалось, мы сократим занимаемое число символов в строке *s
	char *ns;
	if (prtr.char_count < tr.char_count)
	{
		ns = (char *) realloc((void *) *s, ((size_t) prtr.char_count) * sizeof(char)); // sizeof(char) = 1
		*s = ns;
	}
	return prtr;
}

/* Преобразует массив байт byte_array длиной byte_count в строку
и печатает её в файл fp. Формат преобразования - по-умолчанию, смещение адреса - address.
Возвращает результат преобразования с числом записанных символов */
struct Trans_Result fhexprn(FILE *fp, byte *byte_array, size_t byte_count, word address)
{
	char **s; 
        struct Trans_Result prtr2; prtr2.str_count = -1;
        
	// проверка аргументов
	if (fp == NULL || byte_array == NULL)
		return prtr2;
	
	// выделение памяти под указатель на массив символов для преобразования
	s = (char **) malloc(sizeof(char **)); 
	if (s == NULL)
		return prtr2;

	// преобразование с параметрами по-умолчанию
	prtr2 = shexprn(s, byte_array, byte_count, address);
	if (prtr2.char_count <= 0)
	{
            free(s); // помним, что *s была освобождена в default_to_strings()
            prtr2.str_count = -1;
            return prtr2;
	}

	// вывод результата преобразования
	prtr2.char_count = fprintf(fp, "%s", *s);

	// освобождение памяти
	free(*s); // памяти строки
	free(s);  // указателя на строку

	return prtr2;
}

/* Преобразует массив байт byte_array длиной byte_count в массив символов и записывает его
в стандартное устройство вывода stdout. Формат преобразования - по-умолчанию, смещения адреса нет.
Возвращает число записанных в файл fp символов. */
inline struct Trans_Result hexprn(byte *byte_array, size_t byte_count)
{
	return fhexprn(stdout, byte_array, byte_count, 0);
}