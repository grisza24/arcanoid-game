#include <windows.h>
#include <fstream>
#include <string>

using namespace std;

#define ABS(a) ((a) >= 0 ? (a) : -(a))

// Zmienne globalne
HWND hWnd; // uchwyt okna
int const SZEROKOSC_OKNA = 800;
int const WYSOKOSC_OKNA = 600;
HDC ekran; // uchwyt urz¹dzenia okna, po którym bêdziemy rysowaæ
HDC e1, e2;			// rysowaæ bêdziemy na zmianê, raz na e1 raz na e2, po czym bêdziemy dane 'e' rysowaæ na ekranie, stosuj¹c takie powójne buforowanie zapobiegniemy miganiu obrazu
HBITMAP bmp1, bmp2; // niezbêdne do utworzenia e1 i e2
bool ktoryE; // true - rysujemy na e1, false -  rysujemy na e2

HFONT czcionka; // czcionka napisów
RECT rectNapis; // prost¹k¹t w którym bêdzie znajdowa³ siê tekst
wchar_t napis[256]; // bufor na tekst który bêdziemy chcieli wyœwietlaæ (w grze stosuje wchar_t dla lepszego kodowania znaków)

// Klasy
class CObiekt // klasa reprezentuj¹ca obiekt w grze
{
	protected:
		float x, y; // wspó³rzêdne
		int szerokosc, wysokosc; // wymiary
		HBITMAP bmp, bmpM; // bitmapa obrazka i bitmapa maski (dzieki masce mo¿emy nie rysowac niektórych pixeli obrazka)
		HDC hdc, hdcM; // niezbedne do utworzenia bmp i bmpM

	public:
		CObiekt(float x, float y, int s, int w, wstring obrazek, wstring maska) // konstruktor
		{
			this->x = x;
			this->y = y;
			szerokosc = s;
			wysokosc = w;
			hdc = CreateCompatibleDC(ekran); // tworzenie uchwytu urz¹dzenia, który bêdzie zawiera³ nasz¹ bitmapê
			bmp = (HBITMAP)LoadImage(NULL, obrazek.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE); // wczytywanie obrazka
			SelectObject(hdc, bmp); // kojarzenie uchwytu z bitmapa
			if(maska != L"") // jezeli sciezka do pliku z maska nie jest pusta (L"" oznacza, ¿e napis pusty "" ma byæ traktowany w formacie wchar_t[] a nie char[]
			{
				hdcM = CreateCompatibleDC(ekran); // tworzenie uchwytu urzadzenia, który bêdzie zawiera³ maske
				bmpM = (HBITMAP)LoadImage(NULL, maska.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE); // wczytywanie bitmapy z maska
				SelectObject(hdcM, bmpM); // kojarzenie uchwytu z bitmap
			}
			else // jesli sciezka do maski jest pusta
			{
				hdcM = NULL;
				bmpM = NULL; // dzieki tym NULLom bêdziemy widzieli, ¿e nie mamy maski
			}			
		}
		~CObiekt() // destruktor
		{
			DeleteDC(hdc);
			DeleteObject(bmp);
			if(hdcM != NULL) DeleteDC(hdcM);
			if(bmpM != NULL) DeleteObject(bmpM);
		}
		float DajX() { return x; }
		float DajY() { return y; }
		int DajSzerokosc() { return szerokosc; }
		int DajWysokosc() { return wysokosc; }
		void UstawX(float x) { this->x = x; }
		void UstawY(float y) { this->y = y; }
		void UstawSzerokosc(int s) { szerokosc = s;  }
		void UstawWysokosc(int w) { wysokosc = w; }
		virtual void Rysuj(HDC cel) // rysowanie
		{
			if(bmpM != NULL) // jezeli wczytalsmy maske
			{
				BitBlt(cel, (int)x, (int)y, szerokosc, wysokosc, hdcM, 0, 0, SRCAND); // narysuj maskê z flag¹ AND
				BitBlt(cel, (int)x, (int)y, szerokosc, wysokosc, hdc, 0, 0, SRCPAINT); // narysuj maskê z flag¹ PAINT dziêki czemu tam gdzie w masce jest bia³y kolor, a w obrazku czarny tam nie bêdzie nic rysowane
			}
			else BitBlt(cel, (int)x, (int)y, szerokosc, wysokosc, hdc, 0, 0, SRCCOPY); // jesli nie wczytalismy maski to po prostu przekupiuj wszystkie pixele z obrazka
		}
};

class CKafel : public CObiekt
{
	private:
		bool aktywny; // true - widoczny, istnieje, false - zbity

	public:
		CKafel(float x, float y, int s, int w) : CObiekt(x, y, s, w, L"kafel.bmp", L"") { Aktywuj(); }
		bool CzyAktywny() { return aktywny; }
		void Aktywuj() { aktywny = true; }
		void Dezaktywuj() { aktywny = false; }
		void Rysuj(HDC cel)
		{
			if(aktywny == true) CObiekt::Rysuj(cel); // rysuj kafelek tylko wtedy gdy jest aktywny
		}
};

class CPaletka : public CObiekt
{
	public:
		CPaletka(float x, float y, int s, int w) : CObiekt(x, y, s, w, L"paletka.bmp", L"") {}
};

class CPilka : public CObiekt
{
	private:
		float dx, dy; // predkosc

	public:
		CPilka(float x, float y, int s, int w, float dx, float dy) : CObiekt(x, y, s, w, L"pilka.bmp", L"pilkaM.bmp")
		{
			this->dx = dx;
			this->dy = dy;
		}
		float DajDX() { return dx; }
		float DajDY() { return dy; }
		void UstawDX(float dx) { this->dx = dx; }
		void UstawDY(float dy) { this->dy = dy; }
		void Przesun(float wsp) // wsp to czas w milisekundach generowania ostatniej klatki, dzieki czemu na kazdym komputerze gra dzialac bedzie z ta sama szybkoscia, wiecej w "petli glownej"
		{
			x += dx*wsp;
			y += dy*wsp;
		}
		bool CzyKolizja(CObiekt *obiekt)
		{
			// je¿eli nast¹pi³a kolizja
			if(x < obiekt->DajX()+obiekt->DajSzerokosc() && x+szerokosc > obiekt->DajX() &&
			   y < obiekt->DajY()+obiekt->DajWysokosc() && y+wysokosc > obiekt->DajY())
			{
				// oblicz ile pilka "wbi³a" siê w obiekt w osi x oraz y
				float xx, yy;
				if(dx <= 0) xx = obiekt->DajX() + obiekt->DajSzerokosc() - x;
				else xx = obiekt->DajX() - x - szerokosc;
				if(dy <= 0) yy = obiekt->DajY() + obiekt->DajWysokosc() - y;
				else yy = obiekt->DajY() - y - wysokosc;

				// oblicz czy kolizja nastapila najpierw w osi pionowej czy poziomej
				if(dx == 0) xx = 0;
				else xx = ABS(xx/dx);
				if(dy == 0) yy = 0;
				else yy = ABS(yy/dy);

				if(dx == dy && dx == 0) return false;

				// i odpowienio odbij
				if(xx <= yy)
				{
					x -= dx*xx;
					y -= dy*xx;
					dx = -dx;
				}
				if(xx >= yy)
				{
					x -= dx*yy;
					y -= dy*yy;
					dy = -dy;
				}

				return true;
			}
			else return false;
		}
};

// Funkcje
LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
void Rysuj();
bool Aktualizuj(float);
bool WczytajPoziom(int);
void InicjujZasoby();
void ZwolnijZasoby();

// Zmienne globalne gry
CPaletka *paletka;
CPilka *pilka;
const int PLANSZA_RZEDY = 5; // ilosc rzedow kafelek
const int PLANSZA_KOLUMNY = 7; // ilosc kolumn kafelek
CKafel *plansza[PLANSZA_RZEDY][PLANSZA_KOLUMNY]; // tablica reprezentujaca kafelki
CObiekt *tlo;
int zycia; // ilosc zyc
int nrPoziomu; // numer poziomu

int __stdcall WinMain(HINSTANCE hInst, HINSTANCE, LPSTR cmd, int style)
{
	// tworzenie klasy okna
	WNDCLASSEX wndClass;
	wndClass.hInstance = hInst;
	wndClass.lpszClassName = L"klasaOkna";
	wndClass.lpfnWndProc = WindowProc;
	wndClass.style = CS_DBLCLKS;
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndClass.lpszMenuName = NULL;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);

	// tworzenie i wyswietlanie okna
	if(!RegisterClassEx(&wndClass)) return NULL;
	int obramowanieY, obramowanieX;
	obramowanieX = GetSystemMetrics(SM_CXFRAME) * 2; // obliczanie szerokoœci obramowañ po lewej i prawej stronie okna
	obramowanieY = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFRAME)* 2; // obliczanie wysokosci paska okna i dolnego obramowania
	hWnd = CreateWindowEx(0, L"klasaOkna", L"Arkanoid", WS_OVERLAPPEDWINDOW, 0, 0, SZEROKOSC_OKNA+obramowanieX, WYSOKOSC_OKNA+obramowanieY, NULL, NULL, hInst, NULL);
	ShowWindow(hWnd, style);
	
	InicjujZasoby();
	WczytajPoziom(nrPoziomu);
	
	// inicjowanie zmiennych dotycz¹cych licznika dlugosci renderowania jednej klatki
	LARGE_INTEGER czestotliwosc, tiki;
	float nowyCzas, staryCzas, deltaCzasu;
	QueryPerformanceFrequency(&czestotliwosc);
	QueryPerformanceCounter(&tiki);
	nowyCzas = 0.0f;
	staryCzas = (float)(tiki.QuadPart / (double)czestotliwosc.QuadPart);
	
	float przerwa = 0; // przerwa miedzy poziomami

	// pêtla komunikatów
	MSG msg;
    while(1)
    {
		// je¿eli jest jakiœ komunikat systemowy to go obrób
        if(PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE))
        {
			if(!GetMessage(&msg, 0, 0, 0)) break;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
		// jeœli nie to...
        else
        {
			// oblicz dlugosc renderowania ostatniej klatki
			QueryPerformanceCounter(&tiki);
			nowyCzas = (float)(tiki.QuadPart / (double)czestotliwosc.QuadPart);
			deltaCzasu = nowyCzas - staryCzas;
			staryCzas = nowyCzas;

            if(Aktualizuj(deltaCzasu) == false) // jesli funkcja aktualizuj zwrocila falsz to znaczy ze albo graczowi skonczyly sie zycia, albo gracz zbil wszystkie kafelki
			{
				if(zycia == 0) break; // jesli gracz stracil wszystkie zycia to przewij petle a tym samym zakoncz gre

				// jesli petla nie zostala przerwana to odczekaj 4 sekundy
				przerwa = 4.0f;
				while(przerwa >= 0.0f)
				{
					QueryPerformanceCounter(&tiki);
					nowyCzas = (float)(tiki.QuadPart / (double)czestotliwosc.QuadPart);
					deltaCzasu = nowyCzas - staryCzas;
					przerwa -= deltaCzasu;
					staryCzas = nowyCzas;
				}

				// po czym wczytaj wczytaj kolejny poziom
				nrPoziomu++;
				if(WczytajPoziom(nrPoziomu) == false) break; // jesli nie udalo sie znalesc pliku nastepnego poziomu, znaczy, ze gracz przeszedl juz wszystkie plansze, wiec przerwij petle po czym zakoncz gre
			}
			Rysuj();
        }
    }

	ZwolnijZasoby();

	return 0;
}

void Rysuj()
{
	HDC *aktualneHDC = (ktoryE == true ? &e1 : &e2); // ustaw wskaznik aktualneHDC odpowiednio na e1 jesli poprzedni¹ klatkê rysowalismy na e2, lub na e2 w przeciwnym wypadku
	tlo->Rysuj(*aktualneHDC); // rysuj tlo na aktualnym "e"
	for(int i = 0; i < PLANSZA_RZEDY; i++)
	{
		for(int j = 0; j < PLANSZA_KOLUMNY; j++)
		{
			plansza[i][j]->Rysuj(*aktualneHDC); // rysuj kafelki na aktualnym "e"
		}
	}
	paletka->Rysuj(*aktualneHDC); // rysuj paletke
	pilka->Rysuj(*aktualneHDC); // rysuj pileczke

	// wyswietl napis informujacy o ilosci ¿yæ i aktualnym numerze poziomu
	swprintf_s(napis, L"¯ycia: %d\nPoziom: %d", zycia, nrPoziomu);
	DrawText(*aktualneHDC, napis, -1, &rectNapis, DT_LEFT);

	// przekopiuj pixele z aktualnego "e" na ekran, czyli na uchwyt urzadzenia okna
	BitBlt(ekran, 0, 0, SZEROKOSC_OKNA, WYSOKOSC_OKNA, *aktualneHDC, 0, 0, SRCCOPY);

	// zmien na którym "e" mamy rysowaæ nastêpn¹ klatkê
	ktoryE = !ktoryE;
}
bool Aktualizuj(float wsp)
{
	pilka->Przesun(wsp); // przesun pileczke
	bool koniec = true;
	bool koniecPetli = false;
	// przejdz po wszystkich kafelkach...
	for(int i = 0; i < PLANSZA_RZEDY; i++)
	{
		for(int j = 0; j < PLANSZA_KOLUMNY; j++)
		{
			if(plansza[i][j]->CzyAktywny() == true) // ...i jesli kafelek o wspó³rzêdnych ij jest aktywny...
			{
				koniec = false; // zaznacz, ¿e s¹ jeszcze kafelki na planszy
				if(pilka->CzyKolizja((CObiekt*)plansza[i][j]) == true) // ... i pileczka jest z nim w kolicji to:
				{
					plansza[i][j]->Dezaktywuj(); // ukryj kafelek
					koniecPetli = true; // zaznacz, ¿e ju¿ wykryto kolizjê i mo¿na przerwaæ petle
					break;
				}
			}
		}
		if(koniecPetli == true) break;
	}

	if(pilka->CzyKolizja(paletka) == true && pilka->DajY()+pilka->DajWysokosc() >= paletka->DajY()) // je¿eli pi³ka koliduje z paletk¹ i koliduje z ni¹ z góry to
	{
		// wyznacz miejsce uderzenia pileczki w paletke tak ¿e -0.5 to uderzenie pileczki w skrajnie lewy brzeg paletki, a 0.5 w skrajnie prawy
		float wsp = (pilka->DajX()-paletka->DajX())/(paletka->DajSzerokosc()-pilka->DajSzerokosc());
		if(wsp < 0.0f) wsp = 0.0f;
		else if(wsp > 1.0f) wsp = 1.0f;
		wsp -= 0.5f;

		// wyznacz kat pod jakim ma siê odbiæ pileczka tak, ¿e dla kolizji ze skrajnie lewym brzegiem paletki kat odbicia wynosi -45 stopnie (licz¹c od prostej prostopadlej do paletki), a dla skrajeni prawego brzegu 45 stopni, posrednie stany s¹ liniowe wszglêdem po³o¿enia, czyli np. na samym œrodku pileczka odbije siê pod katem 0 stopni czyli dokladnie pionowo
		float dx = 350*sin(3.1415*wsp*0.5f);
		float dy = -350*cos(3.1415*wsp*0.5f);
		pilka->UstawDX(dx);
		pilka->UstawDY(dy);
	}
	if(pilka->DajX() < 0) // je¿eli pileczka uderzyla w lewy brzeg ekranu
	{
		pilka->UstawX(0);
		pilka->UstawDX(-pilka->DajDX());
	}
	else if(pilka->DajX() + pilka->DajSzerokosc() > SZEROKOSC_OKNA) // lub w prawy
	{
		pilka->UstawX((float)(SZEROKOSC_OKNA - pilka->DajSzerokosc()));
		pilka->UstawDX(-pilka->DajDX());
	}
	if(pilka->DajY() < 0) // lub w gorn¹ krawedŸ
	{
		pilka->UstawY(0);
		pilka->UstawDY(-pilka->DajDY());
	}
	// to odbij odpowienio pileczkê

	else if(pilka->DajY() > WYSOKOSC_OKNA + 50) // jeœli pileczka przekroczyla o 50 pixseli doln¹ krawêdŸ ekrany to
	{
		zycia--; // odejmij jedno zycie
		
		// ustaw pileczke na paletce
		pilka->UstawX(paletka->DajX()+(paletka->DajSzerokosc()-pilka->DajSzerokosc())/2);
		pilka->UstawY(paletka->DajY()-pilka->DajSzerokosc());
		// zatrzymaj pileczke
		pilka->UstawDX(0.0f);
		pilka->UstawDY(0.0f);
	}

	if(zycia == 0) return false;
	return !koniec;
}
bool WczytajPoziom(int nr)
{
	ifstream plik;
	wchar_t nazwa[256];
	swprintf_s(nazwa, L"lvl%d.txt", nr);
	plik.open(nazwa);
	if(plik) // jesli istnieje plik poziomu o zadanym numerze to wczytaj kombinacje kafli
	{
		int x;
		for(int i = 0; i < PLANSZA_RZEDY; i++)
		{
			for(int j = 0; j < PLANSZA_KOLUMNY; j++)
			{
				plik >> x;
				if(x == 0) plansza[i][j]->Dezaktywuj(); // zero w pliku oznacza brak kafla
				else plansza[i][j]->Aktywuj(); // jeden oznacza kafla
			}
		}
		plik.close();
		// po wczytaniu poziomu ustaw pileczke na paletce
		pilka->UstawX(paletka->DajX()+(paletka->DajSzerokosc()-pilka->DajSzerokosc())/2);
		pilka->UstawY(paletka->DajY()-pilka->DajSzerokosc());
		// i zatrzymaj pileczke
		pilka->UstawDX(0.0f);
		pilka->UstawDY(0.0f);
		return true;
	}
	return false; // jesli plik nie istnieje to zaznacz, ze gracz przeszedl wszystkie poziomy
}
void InicjujZasoby()
{
	ekran = GetDC(hWnd); // pobierz uchwyt urzadzenia okna po ktorym bedziemy rysowac

	// utworz e1
	e1 = CreateCompatibleDC(ekran);
	bmp1 = CreateCompatibleBitmap(ekran, SZEROKOSC_OKNA, WYSOKOSC_OKNA);
	SelectObject(e1, bmp1);
	// utworz e2
	e2 = CreateCompatibleDC(ekran);
	bmp2 = CreateCompatibleBitmap(ekran, SZEROKOSC_OKNA, WYSOKOSC_OKNA);
	SelectObject(e2, bmp2);
	// zaznacz ze najpierw bedziemy rysowaæ na e1
	ktoryE = true;

	// utworz tlo
	tlo = new CObiekt(0, 0, SZEROKOSC_OKNA, WYSOKOSC_OKNA, L"tlo.bmp", L"");

	// utworz paletke
	const int SZEROKOSC_PALETKI = 200;
	const int WYSOKOSC_PALETKI = 20;
	paletka = new CPaletka((SZEROKOSC_OKNA-SZEROKOSC_PALETKI)/2, WYSOKOSC_OKNA-WYSOKOSC_PALETKI-50, SZEROKOSC_PALETKI, WYSOKOSC_PALETKI);

	// utworz pileczke
	const int ROZMIAR_PILKI = 20;
	pilka = new CPilka(paletka->DajX()+(paletka->DajSzerokosc()-ROZMIAR_PILKI)/2, paletka->DajY()-ROZMIAR_PILKI-50, ROZMIAR_PILKI, ROZMIAR_PILKI, 0, 0);

	// utworz kafelki
	for(int i = 0; i < PLANSZA_RZEDY; i++)
	{
		for(int j = 0; j < PLANSZA_KOLUMNY; j++)
		{
			plansza[i][j] = new CKafel(20+110.0f*j, 20+40.0f*i, 100, 35);
		}
	}

	zycia = 3; // ustaw iloœæ ¿yæ

	czcionka = CreateFont(20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, L"Courier New"); // utworz czcionke
	SetRect(&rectNapis, 15, 550, 250, 600); // ustaw prostok¹t w którym bêdziemy pisaæ
	SetBkMode(e1, TRANSPARENT); // ustaw, ¿eby tekst nie mia³ t³a
	SetBkMode(e2, TRANSPARENT); // --- || ---
	SetTextColor(e1, RGB(255, 255, 255)); // czcionka mia³a kolor bia³y
	SetTextColor(e2, RGB(255, 255, 255)); // --- || ---
	SelectObject(e1, czcionka); // skojarz czcionke z urzadzeniem
	SelectObject(e2, czcionka); // --- || ---

	nrPoziomu = 1; // ustaw poziom na pierwszy
}
void ZwolnijZasoby()
{
	delete tlo;
	delete paletka;
	delete pilka;
	for(int i = 0; i < PLANSZA_RZEDY; i++)
	{
		for(int j = 0; j < PLANSZA_KOLUMNY; j++) delete plansza[i][j];
	}
	DeleteDC(e1);
	DeleteObject(bmp1);
	DeleteDC(e2);
	DeleteObject(bmp2);
	DeleteObject(czcionka);
	DeleteDC(ekran);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if(msg == WM_DESTROY) // jezeli otrzymano komunikat o niszczeniu okna
	{
		PostQuitMessage(0);
		return 0;
	}
	else if(msg == WM_MOUSEMOVE) // jesli ruszono myszk¹ to
	{
		// ustaw pozycje paletki na podstawie pozycji kursora
		paletka->UstawX(LOWORD(lParam)-paletka->DajSzerokosc()*0.5f);
		if(paletka->DajX() < 0) paletka->UstawX(0);
		else if(paletka->DajX() + paletka->DajSzerokosc() > SZEROKOSC_OKNA) paletka->UstawX((float)(SZEROKOSC_OKNA - paletka->DajSzerokosc()));

		// jesli pileczka sie nie porusza to znaczy ze lezy na paletce wiec przesun pileczke razem z paletka
		if(pilka->DajDX() == 0.0f && pilka->DajDY() == 0.0f)
		{
			pilka->UstawX(paletka->DajX()+(paletka->DajSzerokosc()-pilka->DajSzerokosc())/2);
			pilka->UstawY(paletka->DajY()-pilka->DajSzerokosc());
		}

		return 0;
	}
	else if(msg == WM_LBUTTONDOWN) // jezeli klikniêto lewy przycisk myszy i pilka sie nie porusza to rozpedz pilke
	{
		if(pilka->DajDX() == 0.0f && pilka->DajDY() == 0.0f)
		{
			pilka->UstawDX(250);
			pilka->UstawDY(-250);
		}
		return 0;
	}
	else return DefWindowProc(hwnd, msg, wParam, lParam); // w innych wypadkach obsluz domyslnie komunikaty
}