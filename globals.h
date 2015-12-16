
#define SQR(x) ((x)*(x))	/* macro for square */
#ifndef M_PI			/* in case M_PI not found in math.h */
#define M_PI 3.1415927
#endif
#ifndef M_E
#define M_E 2.718282
#endif

#define MAX_FILENAME_CHARS	320

#define MAX_QUEUE 10000

char	filename[MAX_FILENAME_CHARS];

HWND	MainWnd;

		// Display flags
//int		SetPredicates;
HWND hwnd = NULL;
int		redActive;
int		greenActive;
int		blueActive;
int		playModeActive;
int		stepModeActive;
int		red = 0, green = 255, blue = 0;
int		flag = 0;
int intensity_predicate = 15;
int centroid_predicate = 80;
int oneRegionGrowActive = 0;
int jCounter = 0;
int tempJCounter = 0;
char temp1[20];
char temp2[20];
typedef struct MyData {
	unsigned char *image;
	int ROWS1;
	int COLS1;
	int r;
	int c;
	int intensity_thresh;
	int centroid_thresh;
} MYDATA, *PMYDATA;

		// Image data
unsigned char	*OriginalImage;
int				ROWS,COLS;

#define TIMER_SECOND	1			/* ID of timer used for animation */

		// Drawing flags
int		TimerRow,TimerCol;
int		ThreadRow,ThreadCol;
int		ThreadRunning;

		// Function prototypes
LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);
void PaintImage();
DWORD WINAPI RegionGrow(LPVOID lpParam);
void AnimationThread(void *);		/* passes address of window */
void ControlThread(void *);
BOOL CALLBACK GoToProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SetPredicateProc(HWND hwndDlg,UINT message,WPARAM wParam,LPARAM lParam);