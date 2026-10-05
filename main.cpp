#include <iostream>
#include <windows.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cstdio>

using namespace std;

//ширина и высота игрового поля в клетках
const int fid_width = 20;
const int fid_height = 30;

//ширина и высота экрана в символах
const int scr_width = fid_width * 2;
const int scr_height = fid_height;

const char c_fig = 219;
const char c_field = 176; //отображает пустую клетку
const char c_figdown = 178; //приземлившийся объект

typedef char TScreenMap[scr_height][scr_width]; //вывод символов на экран
typedef char TFieldMap[fid_height][fid_width]; //хранит клетки игрового поля

const int shape_width = 4;
const int shape_height = 4;
typedef char TShape[shape_height][shape_width];


//массив хранит в себе фигуры
char *shpArr[] = {
     (char*)".....**..**.....", //квадрат
     (char*)"....****........", //линия
     (char*)"....***..*......", //Т
     (char*)".....***.*......", //L
     (char*)".....**.**......"};//S
const int shpArrCnt = sizeof(shpArr) / sizeof(shpArr[0]);

//курсор в заданной точке экрана
void SetCursPos(int x, int y)
{
    COORD coord;
    coord.X= x;
    coord.Y = y;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
}

class TScreen
{
    void SetEnd() { scr [scr_height-1][scr_width-1] = '\0'; } //двумерный массив символов который появится на экране
public:
    TScreenMap scr;
    TScreen() { Clear(); }
    void Clear() { memset(scr, '.', sizeof(scr)); } //заполняет всё поле точками
    void Show() { SetCursPos(0,0); SetEnd(); cout << scr[0];} //выводит массив на экран
};

class TField {
public:
    TFieldMap field;
    TField() { Clear(); };
    void Clear() { memset(field, c_field, sizeof(field));}
    void Put (TScreenMap &scr);
    void Burn(); //сгорают строки
    };

class TFigure
{
    int x,y; //положение фигуры
    TShape vid; //форма фигуры
    char turn;
    COORD coord[shape_width * shape_height];
    int coordCnt;
    TField *field = 0;
public:
    TFigure(){ memset(this, 0, sizeof(*this)); }
    void FieldSet(TField * _field) { field = _field; }
    void Shape(const char* _vid) { memcpy(vid, _vid, sizeof(vid));} //символы для формы фигуры
    void Pos(int _x, int _y) { x = _x; y = _y; CalcCoord();}; //позиция фигуры
    char TurnGet() { return turn ;};
    void TurnSet( char _turn);
    void Put(TScreenMap &scr); //помещает фигуру в экранный буфер
    void Put(TFieldMap &field);
    bool Move(int dx, int dy);
    int Check();
private:
    void CalcCoord();
};

class TGame
{
    TScreen screen;
    TField field;
    TFigure figure;
    int trn;
public:
    TGame();
    void PlayerControl();
    void Move();
    void Show();
};

TGame::TGame()
{
    trn = 0;
    figure.FieldSet(&field);
    figure.Shape(shpArr[rand() % shpArrCnt]); //выбирает случайный объект из массива
    figure.Pos(fid_width / 2 - shape_width / 2, 0);
}

void TGame::PlayerControl()
{
    if (GetKeyState('W') < 0) trn +=1; else trn = 0; //turn
    if (trn == 1) figure.TurnSet(figure.TurnGet() + 1);
    if (GetKeyState('S') < 0) figure.Move(0, 1); //down
    if (GetKeyState('A') < 0) figure.Move(-1, 0); //right
    if (GetKeyState('D') < 0) figure.Move(1, 0); //left
}

void TGame::Show()
{
    screen.Clear();
    field.Put(screen.scr);
    figure.Put(screen.scr);
    screen.Show();
}

void TGame::Move()
{
    static int tick = 0;
    tick++;
    if (tick >= 5)//двигаем объект каждые 5 итераций
    {
        if (!figure.Move(0,1)) //новая фигура
        {
            figure.Put(field.field);
            figure.Shape(shpArr[rand() % shpArrCnt]);
            figure.Pos(fid_width / 2 - shape_width / 2,0);
            if (figure.Check() > 0) //начинает новую игру если поле заполнено
                field.Clear();
        }
        field.Burn();
        tick = 0;
    }

}

void TFigure::Put(TScreenMap &scr)
{
    for (int i = 0; i < coordCnt; i++)
        scr[coord[i].Y][coord[i].X*2] = scr[coord[i].Y][coord[i].X*2+1]=c_fig;
}

void TFigure::Put(TFieldMap &fld)
{
    for (int i = 0; i < coordCnt; i++)
        fld[coord[i].Y][coord[i].X] = c_figdown;
}

void TFigure::TurnSet(char _turn)
{
    int oldTurn = turn;
    turn = (_turn > 3 ? 0 : (_turn < 0 ? 3 : _turn)); //поворот в 1 из 4 углов
    int chk = Check(); //проверяем столкновение
    if (chk == 0) return; //если всё норм то выходи
    if (chk == 1) //если всё не норм то попробуй повернуть объект 3 раза от границ
    {
        int xx = x;
        int k = (x > (fid_width / 2) ? -1 : +1);
        for (int i = 1; i < 3; i++)
        {
            x += k;
            if (Check() == 0) return; //если получилось то выходи
        }
        x = xx; //если нет то возвращай объект на место и поворот как было
    }
    turn = oldTurn;
    CalcCoord(); //после всего идёт пересчёт координат, считай их в зависимости от угла поворота
}

bool TFigure::Move(int dx, int dy)
{
    int oldX = x, oldY = y;
    Pos(x + dx, y + dy);
    int chk = Check(); //проверяет координаты
    if (chk >= 1) //если вышли за них
    {
        Pos(oldX, oldY); //возвращай обратно
        if (chk == 2) //если всё в норме
            return false; //приземляй объект
    }
    return true; //при успешном перемещении
}

int TFigure::Check()
{
    CalcCoord();
    for (int i = 0; i < coordCnt; i++) //возвращает фигуру обратно
         if (coord[i].X < 0 || coord[i].X >= fid_width)
             return 1;
    for (int i = 0; i< coordCnt; i++) //если она в поле и упала, то она приземляется
        if (coord[i].Y >= fid_height || field->field[coord[i].Y][coord[i].X] == c_figdown)
             return 2;
    return 0;
}

void TFigure::CalcCoord()
{
    int xx, yy;
    coordCnt = 0;
    for (int i = 0; i < shape_width; i++)
        for (int j = 0; j < shape_height; j++)
             if (vid[j][i] == '*')
             { //по часовой стрелке
                 if (turn == 0) xx = x+i, yy = y+j; //меняем перебор клеток фигуры от угла
                 if (turn == 1) xx = x+(shape_height-j-1), yy = y+i; //90
                 if (turn == 2) xx = x+(shape_width-i-1), yy = y+(shape_height-j-1); //180
                 if (turn == 3) xx = x+j, yy=y+(shape_height-i-1) + (shape_width - shape_height); //270
                 coord[coordCnt] = (COORD){(short)xx,(short)yy};
                 coordCnt++;
             }
}

void TField::Put(TScreenMap &scr)
{
    for (int i = 0; i < fid_width; i++)
        for (int j = 0; j < fid_height; j++)
            scr[j][i*2] = scr[j][i*2+1] = field[j][i];
}

void TField::Burn()
{
    for (int j = fid_height-1; j >= 0; j--) //проверяем строку снизу вверх
    {
        static bool fillLine;
        fillLine = true;
        //ищем строку заполненную фигурами
        for (int i = 0; i < fid_width; i++)
            if (field[j][i] != c_figdown)
                //если она есть то двигаем всё что выше на 1 строку вниз
            fillLine = false;
        if (fillLine)
        {
            for (int y = j; y >= 1; y--)
                memcpy(field[y], field[y-1], sizeof(field[y]));
                return;
        }
    }
}

int main()
{
    //ширина и высота клетом и линий в символах
    char command[1000];
    sprintf(command, "mode con cols=%d lines=%d", scr_width, scr_height);
    //командная строка
    system(command);

    srand(time(0));
    TGame game;
    while (1)
    {
        game.PlayerControl();
        game.Move();
        game.Show();
        if (GetKeyState(VK_ESCAPE) < 0) break;
        Sleep(50);
    }
    return 0;
}
