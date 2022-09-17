#define SDL_MAIN_HANDLED

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL.h>
#include <stack>
#include <sstream>
#include <unistd.h>

using namespace std;

std::string FILENAME = "marvel-wallpaper.bmp";
std::string SAIDA = "saida.bmp";

std::string buttonImages[9] =
    {
        "new.bmp",
        "diskette.bmp",
        "linha-diagonal.bmp",
        "retangulo.bmp",
        "hexagono.bmp",
        "circulo.bmp",
        "linha-curva.bmp",
        "fill.bmp",
        "desfazer.bmp"};

// Estrutura para representar pontos
typedef struct
{
    int x, y;
} Point;

typedef struct
{
    Point start, end;
    int actionId;
} Button;

Button buttons[10];
int buttonCount, clickedButtonId = 0;

// vari�veis necess�rias para o SDL
unsigned int *pixels;
int canvasWidth, canvasHeight, width, height;
SDL_Surface *window_surface;
SDL_Surface *imagem;
SDL_Renderer *renderer;

SDL_Window *window;

// T�tulo da janela
std::string titulo = "SDL BMP ";

Uint32 selectedColor = 0;

bool drawingToolBar = false;

bool clickedOnToolBar(Point point)
{
    return point.y > canvasHeight && point.y < height;
}

void takeAction(int action, Point pointClicked);

SDL_Surface *getImage(std::string imagePath, int targetWidth, int targetHeight)
{
    SDL_Surface *sourceImage = SDL_LoadBMP(imagePath.c_str());

    if (sourceImage)
    {
        int width = sourceImage->w;
        int height = sourceImage->h;

        SDL_Rect sourceDimensions;
        sourceDimensions.x = 0;
        sourceDimensions.y = 0;
        sourceDimensions.w = width;
        sourceDimensions.h = height;

        float scaleW = (float)targetWidth / (float)width;
        float scaleH = (float)targetHeight / (float)height;

        SDL_Rect targetDimensions;
        targetDimensions.x = 0;
        targetDimensions.y = 0;
        targetDimensions.w = (int)(width * scaleW);
        targetDimensions.h = (int)(height * scaleH);

        SDL_Surface *surface32 = SDL_CreateRGBSurface(
            sourceImage->flags,
            sourceDimensions.w,
            sourceDimensions.h,
            32,
            sourceImage->format->Rmask,
            sourceImage->format->Gmask,
            sourceImage->format->Bmask,
            sourceImage->format->Amask);

        SDL_BlitSurface(sourceImage, NULL, surface32, NULL);

        SDL_Surface *scaleSurface = SDL_CreateRGBSurface(
            surface32->flags,
            targetDimensions.w,
            targetDimensions.h,
            surface32->format->BitsPerPixel,
            surface32->format->Rmask,
            surface32->format->Gmask,
            surface32->format->Bmask,
            surface32->format->Amask);

        SDL_BlitScaled(surface32, NULL, scaleSurface, NULL);

        SDL_FreeSurface(sourceImage);

        sourceImage = scaleSurface;

        SDL_FreeSurface(surface32);
        surface32 = NULL;

        return sourceImage;
    }

    return NULL;
}

// Gera uma estrutura Point a partir de valores para x e y
Point getPoint(int x, int y)
{
    Point p;
    p.x = x;
    p.y = y;
    return p;
}

// Obt�m a cor de um pixel de uma determinada posi��o
Uint32 getPixel(Point position)
{
    int x = position.x;
    int y = position.y;

    if ((x >= 0 && x <= width) && (y >= 0 && y <= height))
        return pixels[x + width * y];
    else
        return -1;
}

// seta um pixel em uma determinada posi��o,
// atrav�s da coordenadas de cor r, g, b, e alpha (transpar�ncia)
// r, g, b e a variam de 0 at� 255
void setPixel(int x, int y, int r, int g, int b, int a)
{
    if (x < canvasWidth && y < canvasHeight)
        pixels[x + y * width] = SDL_MapRGBA(window_surface->format, r, g, b, a);
}

// seta um pixel em uma determinada posi��o,
// atrav�s da coordenadas de cor r, g e b
// r, g, e b variam de 0 at� 255
void setPixel(int x, int y, int r, int g, int b)
{
    setPixel(x, y, r, g, b, 255);
}

void setPixel(Point point, int r, int g, int b)
{
    setPixel(point.x, point.y, r, g, b, 255);
}

// Mostra na barra de t�tulo da janela a posi��o
// corrente do mouse
void showMousePosition(SDL_Window *window, int x, int y)
{
    std::stringstream ss;
    ss << titulo << " X: " << x << " Y: " << y;
    SDL_SetWindowTitle(window, ss.str().c_str());
}

// Imprime na console a posi��o corrente do mouse
void printMousePosition(int x, int y)
{
    printf("Mouse on x = %d, y = %d\n", x, y);
}

// seta um pixel em uma determinada posi��o,
// atrav�s de um Uint32 representando
// uma cor RGB
void setPixel(int x, int y, Uint32 color)
{
    if ((x >= 0 && x < canvasWidth && y >= 0 && y < canvasHeight) || (drawingToolBar && x >= 0 && x < width && y >= 0 && y < height))
    {
        pixels[x + y * width] = color;
    }
}

// retorna uma cor RGB(UInt32) formada
// pelas componentes r, g, b e a(transpar�ncia)
// informadas. r, g, b e a variam de 0 at� 255
Uint32 RGB(int r, int g, int b, int a)
{
    return SDL_MapRGBA(window_surface->format, r, g, b, a);
}

// retorna uma cor RGB(UInt32) formada
// pelas componentes r, g, e b
// informadas. r, g e b variam de 0 at� 255
// a transpar�ncia � sempre 255 (imagem opaca)
Uint32 RGB(int r, int g, int b)
{
    return RGB(r, g, b, 255);
}

// retorna um componente de cor de uma cor RGB informada
// aceita os par�metros 'r', 'R','g', 'G','b' e 'B',
Uint8 getColorComponent(Uint32 pixel, char component)
{
    Uint32 mask;

    switch (component)
    {
    case 'b':
    case 'B':
        mask = RGB(0, 0, 255);
        pixel = pixel & mask;
        break;
    case 'r':
    case 'R':
        mask = RGB(255, 0, 0);
        pixel = pixel & mask;
        pixel = pixel >> 16;
        break;
    case 'g':
    case 'G':
        mask = RGB(0, 255, 0);
        pixel = pixel & mask;
        pixel = pixel >> 8;
        break;
    }
    return (Uint8)pixel;
}

void drawBresenham(int x1, int y1, int x2, int y2, Uint32 cor)
{
    int x, y, dx, dy, dx1, dy1, px, py, xe, ye, i;

    dx = x2 - x1;
    dy = y2 - y1;

    dx1 = fabs(dx);
    dy1 = fabs(dy);

    px = 2 * dy1 - dx1;
    py = 2 * dx1 - dy1;

    if (dy1 <= dx1)
    {
        if (dx >= 0)
        {
            x = x1;
            y = y1;
            xe = x2;
        }
        else
        {
            x = x2;
            y = y2;
            xe = x1;
        }
        setPixel(x, y, cor);
        for (i = 0; x < xe; i++)
        {
            x = x + 1;
            if (px < 0)
            {
                px = px + 2 * dy1;
            }
            else
            {
                if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
                {
                    y = y + 1;
                }
                else
                {
                    y = y - 1;
                }
                px = px + 2 * (dy1 - dx1);
            }
            setPixel(x, y, cor);
        }
    }
    else
    {
        if (dy >= 0)
        {
            x = x1;
            y = y1;
            ye = y2;
        }
        else
        {
            x = x2;
            y = y2;
            ye = y1;
        }
        setPixel(x, y, cor);
        for (i = 0; y < ye; i++)
        {
            y = y + 1;
            if (py <= 0)
            {
                py = py + 2 * dx1;
            }
            else
            {
                if ((dx < 0 && dy < 0) || (dx > 0 && dy > 0))
                {
                    x = x + 1;
                }
                else
                {
                    x = x - 1;
                }
                py = py + 2 * (dx1 - dy1);
            }
            setPixel(x, y, cor);
        }
    }
}

void drawBresenham(Point p1, Point p2, Uint32 cor)
{
    return drawBresenham(p1.x, p1.y, p2.x, p2.y, cor);
}

void drawRectangle(int x1, int y1, int x2, int y2, Uint32 color)
{
    drawBresenham(x1, y1, x2, y1, color);
    drawBresenham(x2, y1, x2, y2, color);
    drawBresenham(x1, y2, x2, y2, color);
    drawBresenham(x1, y1, x1, y2, color);
}

void drawRectangle(int x1, int y1, int x2, int y2)
{
    drawRectangle(x1, y1, x2, y2, selectedColor);
}

void drawRectangle(Point start, Point end, Uint32 color)
{
    drawRectangle(start.x, start.y, end.x, end.y, color);
}

void drawRectangle(Point start, Point end)
{
    drawRectangle(start, end, selectedColor);
}

// TRAB1 preencher (REVISAR)
void floodFill(int x, int y, Uint32 newColor, Uint32 oldColor)
{
    if (y < 0 || y > height - 1 || x < 0 || x > width - 1)
    {
        return;
    }

    stack<Point> st;

    st.push(getPoint(x, y));

    while (st.size() > 0)
    {
        Point p = st.top();

        st.pop();

        int x = p.x;
        int y = p.y;

        if (y < 0 || y > height - 1 || x < 0 || x > width - 1)
            continue;

        if (getPixel(getPoint(x, y)) == oldColor)
        {
            setPixel(x, y, newColor);

            st.push(getPoint(x + 1, y));
            st.push(getPoint(x - 1, y));
            st.push(getPoint(x, y + 1));
            st.push(getPoint(x, y - 1));
        }
    }
}

void floodFill(Point p, Uint32 newColor, Uint32 oldColor)
{
    floodFill(p.x, p.y, newColor, oldColor);
}

// Aqui ocorrem as chamadas das fun��es a ser exibidas na janela
void display()
{
    SDL_UpdateWindowSurface(window);
}

int getClickedButton(int x, int y)
{
    int i = 0;

    for (; i < buttonCount; i++)
    {
        Button button = buttons[i];

        if (x > button.start.x && x < button.end.x && y > button.start.y && y < button.end.y)
            return button.actionId;
    }

    return -1;
}

void reset_screen()
{
    for (int y = 0; y < canvasHeight; ++y)
    {
        for (int x = 0; x < canvasWidth; ++x)
        {
            setPixel(x, y, RGB(255, 255, 255));
        }
    }
}

// ----------------------------------------------------------------
// Draw window resources
// ----------------------------------------------------------------
void drawColorPicker()
{
    int imgWidth = 80, imgDepth = 50;

    imagem = getImage("rgb.bmp", imgWidth, imgDepth);

    SDL_Rect rectPos;

    rectPos.x = 550;
    rectPos.y = 492;

    SDL_BlitSurface(imagem, NULL, window_surface, &rectPos);

    Button button;

    Point start;
    Point end;

    start.x = rectPos.x;
    start.y = rectPos.y;

    end.x = start.x + imgWidth;
    end.y = start.y + imgDepth;

    button.start = start;
    button.end = end;

    button.actionId = 8;

    buttons[8] = button;

    buttonCount++;
}

void drawButtons()
{
    int startX = 0;
    int endX = width - 1;

    int startY = canvasHeight + 1;
    int endY = height - 1;

    Uint32 backgroundColor = RGB(100, 50, 255);
    Uint32 buttonColor = RGB(255, 255, 255);

    int buttonSize = 60;

    int buttonOffset = 10;

    drawRectangle(startX, startY, endX, endY, backgroundColor);

    floodFill(startX + buttonOffset, buttonOffset + startY, backgroundColor, 0);

    int i = 0;

    for (; i < 9; i++)
    {
        Button button;

        Point start;
        Point end;

        start.x = (i * buttonSize) + startX + buttonOffset;
        start.y = buttonOffset + startY;

        end.x = (i * buttonSize) + startX + buttonSize;
        end.y = buttonSize + startY;

        button.start = start;
        button.end = end;

        button.actionId = i;

        int sizeOffsetToIcon = 20;
        imagem = getImage(buttonImages[i], buttonSize - sizeOffsetToIcon, buttonSize - sizeOffsetToIcon);

        SDL_Rect rectPos;

        rectPos.x = start.x + sizeOffsetToIcon / 4;
        rectPos.y = start.y + sizeOffsetToIcon / 4;

        drawRectangle(start, end, buttonColor);

        floodFill((i * buttonSize) + startX + buttonOffset * 2,
                  buttonOffset * 2 + startY,
                  buttonColor,
                  backgroundColor);

        SDL_BlitSurface(imagem, NULL, window_surface, &rectPos);

        buttons[buttonCount] = button;

        buttonCount++;
    }
}

void drawToolBar()
{
    drawButtons();
    drawColorPicker();
}

void drawScreen()
{
    drawingToolBar = true;
    drawToolBar();
    drawingToolBar = false;

    display();
}
// ----------------------------------------------------------------

// ----------------------------------------------------------------
// Button actions
// ----------------------------------------------------------------

void drawLine_clicked(Point pointClicked)
{
    int count = 0;
    Point firstPoint, secondPoint;

    while (count < 2)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                /*Se o bot�o esquerdo do mouse � pressionado */
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    if (count == 0)
                        firstPoint = getPoint(event.motion.x, event.motion.y);
                    else
                    {
                        secondPoint = getPoint(event.motion.x, event.motion.y);
                    }

                    count++;
                }
            }
        }
    }

    drawBresenham(firstPoint, secondPoint, selectedColor);
}

void drawRectangle_clicked(Point pointClicked)
{
    int count = 0;
    Point firstPoint, secondPoint;

    while (count < 2)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                /*Se o bot�o esquerdo do mouse � pressionado */
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    if (count == 0)
                        firstPoint = getPoint(event.motion.x, event.motion.y);
                    else
                    {
                        secondPoint = getPoint(event.motion.x, event.motion.y);
                    }

                    count++;
                }
            }
        }
    }

    drawRectangle(firstPoint, secondPoint, selectedColor);
}

void drawPolygon_clicked(Point pointClicked)
{
    int rightCliked = 0;
    int count = 0;
    Point previousPoint, currentPoint, firstPoint;

    while (rightCliked == 0)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                /*Se o bot�o esquerdo do mouse � pressionado */
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    Point pointClickedNow = getPoint(event.motion.x, event.motion.y);

                    if (count == 0)
                    {
                        firstPoint = clickedOnToolBar(pointClicked) == false
                                         ? pointClicked
                                         : pointClickedNow;

                        currentPoint = pointClickedNow;
                    }
                    else
                    {
                        previousPoint = currentPoint;
                        currentPoint = pointClickedNow;

                        drawBresenham(previousPoint, currentPoint, selectedColor);

                        display();
                    }

                    count++;
                }

                if (event.button.button == SDL_BUTTON_RIGHT)
                {
                    drawBresenham(currentPoint, firstPoint, selectedColor);

                    return;
                }
            }
        }
    }
}

void pickColor_clicked(Point pointClicked)
{
    selectedColor = getPixel(pointClicked);
}

void fill_clicked(Point pointClicked)
{
    if (clickedOnToolBar(pointClicked) == false)
    {
        Uint32 currentColor = getPixel(pointClicked);

        floodFill(pointClicked, selectedColor, currentColor);
        return;
    }

    while (true)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                /*Se o bot�o esquerdo do mouse � pressionado */
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    pointClicked = getPoint(event.motion.x, event.motion.y);

                    Uint32 currentColor = getPixel(pointClicked);

                    if (clickedOnToolBar(pointClicked) == false && selectedColor != currentColor)
                        floodFill(pointClicked, selectedColor, currentColor);

                    return;
                }
            }
        }
    }
}

//----------------------------------------------------------------

void takeAction(int action, Point pointClicked)
{
    switch (action)
    {
    case 0:
        reset_screen();
        break;
    case 1:
        break;
    case 2:
        drawLine_clicked(pointClicked);
        break;
    case 3:
        drawRectangle_clicked(pointClicked);
        break;
    case 4:
        drawPolygon_clicked(pointClicked);
        break;
        // case 5:
        //     drawCircle_clicked();
        //     break;
        // case 6:
        //     drawCurve_clicked();
        //     break;
    case 7:
        fill_clicked(pointClicked);
        break;
    case 8:
        pickColor_clicked(pointClicked);
        break;
    // case 9:
    //     undo_clicked();
    //     break;
    default:
        break;
    }
}

// void copia_para_bmp(Bit)
//  Inicializa o SDL, abre a janela e controla o loop
//  principal do controle de eventos
int main()
{
    // Inicializa��es iniciais obrigat�rias

    int result;

    setlocale(LC_ALL, NULL);

    SDL_Init(SDL_INIT_VIDEO);

    window = SDL_CreateWindow(titulo.c_str(),
                              SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              640, 555, 0);
    window_surface = SDL_GetWindowSurface(window);

    pixels = (unsigned int *)window_surface->pixels;
    width = canvasWidth = 640;
    canvasHeight = 480;

    height = canvasHeight + 75;
    // Fim das inicializa��es

    drawScreen();
    reset_screen();
    printf("SDL Pixel format: %s\n",
           SDL_GetPixelFormatName(window_surface->format->format));

    while (1)
    {
        SDL_Event event;

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                exit(0);
            }

            if (event.type == SDL_KEYDOWN)
            {
                switch (event.key.keysym.sym)
                {
                case SDLK_s:

                    result = SDL_SaveBMP(window_surface, SAIDA.c_str());
                    if (result < 0)
                    {
                        printf("Ocorreu um erro salvando o arquivo\n");
                    }
                    else
                    {
                        printf("Arquivo salvo com sucesso!\n");
                    }
                    // saveImg(SAIDA,bmp);
                    break;
                }
            }

            if (event.type == SDL_WINDOWEVENT)
            {
                if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                {
                    display();
                    // SDL_BlitSurface(imagem, NULL, window_surface, &rectPos);
                }
            }

            // Se o mouse � movimentado
            if (event.type == SDL_MOUSEMOTION)
            {
                // Mostra as posi��es x e y do mouse
                showMousePosition(window, event.motion.x, event.motion.y);
            }
            if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                /*Se o bot�o esquerdo do mouse � pressionado */
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    int clickedButton = getClickedButton(event.motion.x, event.motion.y);

                    if (clickedButton != -1)
                        clickedButtonId = clickedButton;

                    takeAction(clickedButtonId, getPoint(event.motion.x, event.motion.y));
                }
            }
        }

        display();
    }
}
