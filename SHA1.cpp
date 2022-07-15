#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

uint32_t f(uint32_t x, uint32_t y, uint32_t z, size_t i);
uint32_t k(size_t i);
string myHash(string text);
string getTextFromFile(string path);

int main()
{
    setlocale(LC_ALL, "ru");

    cout << myHash(getTextFromFile("file")) << endl;

    system("pause");
    return 0;
}

// f_t по стандарту https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf 10-я страница
uint32_t f(uint32_t x, uint32_t y, uint32_t z, size_t i)
{
    if (i < 20)
    {
        // Ch (x, y, z) = (x & y) ^ (~x & z)
        return (x & y) ^ ((~x) & z);
    }
    else if (i < 40)
    {
        // Parity (x, y, z) = x ^ y ^ z
        return x ^ y ^ z;
    }
    else if (i < 60)
    {
        // Maj (x, y, z) = (x & y) ^ (x & z) ^ (y & z)
        return (x & y) ^ (x & z) ^ (y & z);
    }
    else if (i < 80)
    {
        // Parity (x, y, z) = x ^ y ^ z
        return x ^ y ^ z;
    }
    else
    {
        throw;
    }
}

// k_t по стандарту https://nvlpubs.nist.gov/nistpubs/FIPS/NIST.FIPS.180-4.pdf 11-я страница
uint32_t k(size_t i)
{
    if (i < 20)
    {
        return 0x5a827999;
    }
    else if (i < 40)
    {
        return 0x6ed9eba1;
    }
    else if (i < 60)
    {
        return 0x8f1bbcdc;
    }
    else if (i < 80)
    {
        return 0xca62c1d6;
    }
    else
    {
        throw;
    }
}

string myHash(string text)
{
    /*  Preprocessing
     *  добавляю бит "1" и
     *  добавляю нули пока количество бит % 512 не станет равным 448 и
     *  добавляю длину исходного текста (8 байт)
     */
    string modifiedText = text;
    // добавляю байт 10000000 как символ
    modifiedText += (char)0x80;
    // заполняю нулями, чтобы длина была кратна 56 байтам
    while (modifiedText.size() % 64 != 56)
    {
        modifiedText += (char)0x0;
    }
    // добавляю длину исходного текста в битах
    uint64_t initialTextLength = text.size() * 8; // uint64_t - unsigned long long (64 бита)
    modifiedText += (char)(initialTextLength >> 56);
    modifiedText += (char)(initialTextLength >> 48);
    modifiedText += (char)(initialTextLength >> 40);
    modifiedText += (char)(initialTextLength >> 32);
    modifiedText += (char)(initialTextLength >> 24);
    modifiedText += (char)(initialTextLength >> 16);
    modifiedText += (char)(initialTextLength >> 8);
    modifiedText += (char)(initialTextLength);

    /*  Parsing the Message
     *  разбиваю полученный текст на несколько массивов по 64 байта
     *  в каждом массиве содержится 16 (в будущем 80) элементов по 4 байта каждый
     */
     // вот столько будет больших массивов по 64 байта
    size_t chunksLength = modifiedText.size() / 64;
    uint32_t** chunks = new uint32_t * [chunksLength]; // uint32_t - unsigned int (32 бита)
    // цикл по большому массиву
    for (size_t chunk64_i = 0; chunk64_i < chunksLength; chunk64_i++)
    {
        // выделяю память
        chunks[chunk64_i] = new uint32_t[80]; // потому что в будущем 80 элементов
        // цикл по массиву из 16 (80) элементов
        for (size_t chunk4_i = 0; chunk4_i < 16; chunk4_i++)
        {
            // предварительно зануляю переменную, которая 4-е байта
            chunks[chunk64_i][chunk4_i] = 0;
            // побитовое или, фактически запись на X - позиции, если смотреть так
            // 10101010.10101010.10101010.XXXXXXXX
            chunks[chunk64_i][chunk4_i] |= (unsigned char)modifiedText[chunk64_i * 16 + chunk4_i * 4];
            // сдвиг влево на 8 позиций, получается вот так
            // 10101010.10101010.XXXXXXXX.00000000
            chunks[chunk64_i][chunk4_i] = chunks[chunk64_i][chunk4_i] << 8;
            // то же
            chunks[chunk64_i][chunk4_i] |= (unsigned char)modifiedText[chunk64_i * 16 + chunk4_i * 4 + 1];
            chunks[chunk64_i][chunk4_i] = chunks[chunk64_i][chunk4_i] << 8;
            // то же
            chunks[chunk64_i][chunk4_i] |= (unsigned char)modifiedText[chunk64_i * 16 + chunk4_i * 4 + 2];
            chunks[chunk64_i][chunk4_i] = chunks[chunk64_i][chunk4_i] << 8;
            // то же
            chunks[chunk64_i][chunk4_i] |= (unsigned char)modifiedText[chunk64_i * 16 + chunk4_i * 4 + 3];
        }
    }

    /*  Hash Computation
     *  заполняю остальные элементы, которые в будущем 80
     */
     // цикл по большому массиву
    for (size_t chunk64_i = 0; chunk64_i < chunksLength; chunk64_i++)
    {
        // цикл по массиву из 16 (80) элементов
        // от 17-го элемента до 80-го
        for (size_t chunk4_i = 16; chunk4_i < 80; chunk4_i++)
        {
            // XOR 4-х предшествующих элементов
            chunks[chunk64_i][chunk4_i] = chunks[chunk64_i][chunk4_i - 3] ^ chunks[chunk64_i][chunk4_i - 8] ^ chunks[chunk64_i][chunk4_i - 14] ^ chunks[chunk64_i][chunk4_i - 16];
            // циклический сдвиг влево
            chunks[chunk64_i][chunk4_i] = (chunks[chunk64_i][chunk4_i] << 1) | (chunks[chunk64_i][chunk4_i] >> 31);
        }
    }
    // начальные значения хэша
    uint32_t h0, h1, h2, h3, h4;
    h0 = 0x67452301;
    h1 = 0xefcdab89;
    h2 = 0x98badcfe;
    h3 = 0x10325476;
    h4 = 0xc3d2e1f0;
    // переменные, для подсчёта хэша
    uint32_t  a, b, c, d, e, t;
    // цикл по большому массиву
    for (size_t chunk64_i = 0; chunk64_i < chunksLength; chunk64_i++)
    {
        // инициализация рабочих переменных
        a = h0;
        b = h1;
        c = h2;
        d = h3;
        e = h4;
        // цикл по массиву из 80 (16) элементов
        for (size_t chunk4_i = 0; chunk4_i < 80; chunk4_i++)
        {
            // T = ROTL_5(a) + f_t(b, c, d) + e + k_t + W_t
            t = ((a << 5) | (a >> 27)) + f(b, c, d, chunk4_i) + e + k(chunk4_i) + chunks[chunk64_i][chunk4_i];
            e = d;
            d = c;
            // c = ROTL_30(b)
            c = (b << 30) | (b >> 2);
            b = a;
            a = t;
        }
        // промежуточные хэш значения
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }

    // преобразование
    stringstream ss;
    ss << hex << h0 << " " << h1 << " " << h2 << " " << h3 << " " << h4;
    return ss.str();
}

string getTextFromFile(string path)
{
    // считывание файла
    ifstream file;
    // открытие файла в бинарном виде на чтение
    string text = "";
    file.open(path, ios::in | ios::binary);
    if (!file.is_open())
    {
        file.close();
        cout << "Не удалось открыть файл." << endl;
        system("pause");
        return 0;
    }
    // считываю файл
    const size_t readCount = 100;
    char buff[readCount];
    while (!file.eof())
    {
        file.read(buff, readCount);
        text += buff;
    }
    // закрываю его
    file.close();

    return text;
}