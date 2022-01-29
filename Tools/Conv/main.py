import re


def main():
    table = '''{   1080,   1793}, // 0
{   930 ,   1821}, // 1
{   690 ,   1660}, // 2
{   438 ,   1634}, // 3
{   190 ,   1592}, // 4
{   0   ,   0   }, // 5
{   970 ,   1660}, // 6
{   760 ,   1440}, // 7
{   423 ,   1330}, // 8
{   260 ,   1263}, // 9
{   565 ,   1220}, // 10
{   587 ,   1085}, // 11
{   867 ,   941 }, // 12
{   1240,   865 }, // 13
{   589 ,   865 }, // 14
{   230 ,   891 }, // 15
{   900 ,   786 }, // 16
{   582 ,   661 }, // 17
{   285 ,   528 }, // 18
{   585 ,   518 }, // 19
{   410 ,   440 }, // 20
{   800 ,   270 }, // 21
{   1000,   102 }, // 22
{   1110,   -19 }, // 23
{   930 ,   -25 }, // 24
{   673 ,   114 }, // 25
{   386 ,   169 }, // 26
{   176 ,   245 }, // 27'''

    lines = table.split('\n')

    new_table = ""
    min_r = 10000
    min_a = 10000
    max_r = -10000
    max_a = -10000

    for line in lines:
        line = line.replace('{', '')
        line = line.replace('}', '')
        line = ''.join(line.split())
        tokens = line.split(',')
        r = int(tokens[0])
        r = round(r / 13)
        a = int(tokens[1])
        a = round(a/10)
        if r > max_r:
            max_r = r
        if r < min_r:
            min_r = r
        if a > max_a:
            max_a = a
        if a < min_a:
            min_a = a
        new_table = new_table + "    {" + str(r) + ", " + str(a) + "}, " + tokens[2] + "\n"

    new_table = \
        "#define RMIN        " + str(min_r) + "L\n" + \
        "#define RMAX        " + str(max_r+1) + "L\n" + \
        "#define AMIN        " + str(min_a) + "L\n" + \
        "#define AMAX        " + str(max_a+1) + "L\n\n" + \
        "static int ITable[LED_CNT][2] = {\n" + \
        new_table + \
        "};\n"

    print(new_table)


# Press the green button in the gutter to run the script.
if __name__ == '__main__':
    main()
