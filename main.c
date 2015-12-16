
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <sys/timeb.h>
#include <windows.h>
#include <wingdi.h>
#include <winuser.h>
#include <process.h>	/* needed for multithreading */
#include "resource.h"
#include "globals.h"


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPTSTR lpCmdLine, int nCmdShow)

{
	MSG			msg;
	HWND		hWnd;
	WNDCLASS	wc;


	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(hInstance, "ID_PLUS_ICON");
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = "ID_MAIN_MENU";
	wc.lpszClassName = "PLUS";

	if (!RegisterClass(&wc))
		return(FALSE);

	hWnd = CreateWindow("PLUS", "plus program",
		WS_OVERLAPPEDWINDOW | WS_HSCROLL | WS_VSCROLL,
		CW_USEDEFAULT, 0, 400, 400, NULL, NULL, hInstance, NULL);
	if (!hWnd)
		return(FALSE);

	ShowScrollBar(hWnd, SB_BOTH, FALSE);
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
	MainWnd = hWnd;

	redActive = 0;
	greenActive = 1;
	blueActive = 0;
	playModeActive = 1;
	stepModeActive = 0;

	strcpy(filename, "");
	OriginalImage = NULL;
	ROWS = COLS = 0;

	InvalidateRect(hWnd, NULL, TRUE);
	UpdateWindow(hWnd);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return(msg.wParam);
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam)

{
	HMENU				hMenu;
	OPENFILENAME		ofn;
	FILE				*fpt;
	HDC					hDC;
	char				header[320], text[320];
	int					BYTES, xPos, yPos;
	HINSTANCE hinst = NULL;
	HANDLE  hThread;
	DWORD   dwThreadId;

	switch (uMsg)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_RED:
			redActive = 1;
			greenActive = 0;
			blueActive = 0;
			red = 255;
			green = 0;
			blue = 0;
			break;
		case ID_GREEN:
			redActive = 0;
			greenActive = 1;
			blueActive = 0;
			green = 255;
			red = 0;
			blue = 0;
			break;
		case ID_BLUE:
			redActive = 0;
			greenActive = 0;
			blueActive = 1;
			blue = 255;
			red = 0;
			green = 0;
			break;
		case ID_PLAY:
			playModeActive = 1;
			stepModeActive = 0;
			flag = 0;
			break;
		case ID_STEP:
			playModeActive = 0;
			stepModeActive = 1;
			flag = 1;
			break;
		case ID_DISPLAY_SETPREDICATES:
			//dialog box here
			if (DialogBox(hinst,
				MAKEINTRESOURCE(IDD_DIALOG1),
				hWnd,
				(DLGPROC)SetPredicateProc) == IDOKMY)
			{
				intensity_predicate = atoi(temp1);
				centroid_predicate = atoi(temp2);
			}
			break;
		case ID_CLEAR:
			PaintImage();
			break;
		case ID_FILE_LOAD:
			if (OriginalImage != NULL)
			{
				free(OriginalImage);
				OriginalImage = NULL;
			}
			memset(&(ofn), 0, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.lpstrFile = filename;
			filename[0] = 0;
			ofn.nMaxFile = MAX_FILENAME_CHARS;
			ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY;
			ofn.lpstrFilter = "PPM files\0*.ppm\0All files\0*.*\0\0";
			if (!(GetOpenFileName(&ofn)) || filename[0] == '\0')
				break;		/* user cancelled load */
			if ((fpt = fopen(filename, "rb")) == NULL)
			{
				MessageBox(NULL, "Unable to open file", filename, MB_OK | MB_APPLMODAL);
				break;
			}
			fscanf(fpt, "%s %d %d %d", header, &COLS, &ROWS, &BYTES);
			if (strcmp(header, "P5") != 0 || BYTES != 255)
			{
				MessageBox(NULL, "Not a PPM (P5 greyscale) image", filename, MB_OK | MB_APPLMODAL);
				fclose(fpt);
				break;
			}
			OriginalImage = (unsigned char *)calloc(ROWS*COLS, 1);
			header[0] = fgetc(fpt);	/* whitespace character after header */
			fread(OriginalImage, 1, ROWS*COLS, fpt);
			fclose(fpt);
			SetWindowText(hWnd, filename);
			PaintImage();
			break;

		case ID_FILE_QUIT:
			DestroyWindow(hWnd);
			break;
		}
		break;
	case WM_SIZE:		  /* could be used to detect when window size changes */
		PaintImage();
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	case WM_PAINT:
		PaintImage();
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	case WM_KEYDOWN:
		if (wParam == 'j' || wParam == 'J')
			jCounter++;
		break;
	case WM_LBUTTONDOWN:
	{
		//if (oneRegionGrowActive == 0)
		//{
		//	oneRegionGrowActive = 1;
			int xxPos = LOWORD(lParam);
			int yyPos = HIWORD(lParam);

			unsigned char	*image;
			int		r, c;
			int		i;

			image = (unsigned char *)calloc(ROWS*COLS, 1);
			

			for (r = 0; r < ROWS; r++)
				for (c = 0; c < COLS; c++)
					image[r*COLS + c] = OriginalImage[r*COLS + c];

			c = xxPos, r = yyPos;
			PMYDATA pData;
			pData = (PMYDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,sizeof(MYDATA));
			pData->image = image;
			pData->ROWS1 = ROWS;
			pData->COLS1 = COLS;
			pData->r = r;
			pData->c = c;
			pData->centroid_thresh = centroid_predicate;
			pData->intensity_thresh = intensity_predicate;

			hThread = CreateThread(

				NULL,                   // default security attributes
				0,                      // use default stack size  
				RegionGrow,       // thread function name
				pData,          // argument to thread function 
				0,                      // use default creation flags 
				&dwThreadId);
			//RegionGrow(image,ROWS, COLS, r, c,intensity_predicate, centroid_predicate);

			return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		//}
		break;

	}
	case WM_RBUTTONDOWN:
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	case WM_HSCROLL:	  /* this event could be used to change what part of the image to draw */
		PaintImage();	  /* direct PaintImage calls eliminate flicker; the alternative is InvalidateRect(hWnd,NULL,TRUE); UpdateWindow(hWnd); */
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	case WM_VSCROLL:
		PaintImage();
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return(DefWindowProc(hWnd, uMsg, wParam, lParam));
		break;
	}

	hMenu = GetMenu(MainWnd);
	if (redActive == 1)
		CheckMenuItem(hMenu, ID_RED, MF_CHECKED);
	else
		CheckMenuItem(hMenu, ID_RED, MF_UNCHECKED);
	if (greenActive == 1)
		CheckMenuItem(hMenu, ID_GREEN, MF_CHECKED);
	else
		CheckMenuItem(hMenu, ID_GREEN, MF_UNCHECKED);
	if (blueActive == 1)
		CheckMenuItem(hMenu, ID_BLUE, MF_CHECKED);
	else
		CheckMenuItem(hMenu, ID_BLUE, MF_UNCHECKED);
	if (playModeActive == 1)
		CheckMenuItem(hMenu, ID_PLAY, MF_CHECKED);
	else
		CheckMenuItem(hMenu, ID_PLAY, MF_UNCHECKED);
	if (stepModeActive == 1)
		CheckMenuItem(hMenu, ID_STEP, MF_CHECKED);
	else
		CheckMenuItem(hMenu, ID_STEP, MF_UNCHECKED);
	DrawMenuBar(hWnd);

	return(0L);
}

BOOL CALLBACK SetPredicateProc(HWND hwndDlg,
	UINT message,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOKMY:
			if (!GetDlgItemText(hwndDlg, IDC_EDIT1, temp1, 20))
			{
				intensity_predicate = atoi(temp1);
			}
			if(!GetDlgItemText(hwndDlg, IDC_EDIT2, temp2, 20))
				centroid_predicate = atoi(temp2);
			// Fall through. 

		case IDCANCELMY:
			EndDialog(hwndDlg, wParam);
			return TRUE;
		}
	}
	return FALSE;
}

void PaintImage()

{
	PAINTSTRUCT			Painter;
	HDC					hDC;
	BITMAPINFOHEADER	bm_info_header;
	BITMAPINFO			*bm_info;
	int					i, r, c, DISPLAY_ROWS, DISPLAY_COLS;
	unsigned char		*DisplayImage;

	if (OriginalImage == NULL)
		return;		/* no image to draw */

					/* Windows pads to 4-byte boundaries.  We have to round the size up to 4 in each dimension, filling with black. */
	DISPLAY_ROWS = ROWS;
	DISPLAY_COLS = COLS;
	if (DISPLAY_ROWS % 4 != 0)
		DISPLAY_ROWS = (DISPLAY_ROWS / 4 + 1) * 4;
	if (DISPLAY_COLS % 4 != 0)
		DISPLAY_COLS = (DISPLAY_COLS / 4 + 1) * 4;
	DisplayImage = (unsigned char *)calloc(DISPLAY_ROWS*DISPLAY_COLS, 1);
	for (r = 0; r<ROWS; r++)
		for (c = 0; c<COLS; c++)
			DisplayImage[r*DISPLAY_COLS + c] = OriginalImage[r*COLS + c];

	BeginPaint(MainWnd, &Painter);
	hDC = GetDC(MainWnd);
	bm_info_header.biSize = sizeof(BITMAPINFOHEADER);
	bm_info_header.biWidth = DISPLAY_COLS;
	bm_info_header.biHeight = -DISPLAY_ROWS;
	bm_info_header.biPlanes = 1;
	bm_info_header.biBitCount = 8;
	bm_info_header.biCompression = BI_RGB;
	bm_info_header.biSizeImage = 0;
	bm_info_header.biXPelsPerMeter = 0;
	bm_info_header.biYPelsPerMeter = 0;
	bm_info_header.biClrUsed = 256;
	bm_info_header.biClrImportant = 256;
	bm_info = (BITMAPINFO *)calloc(1, sizeof(BITMAPINFO) + 256 * sizeof(RGBQUAD));
	bm_info->bmiHeader = bm_info_header;
	for (i = 0; i<256; i++)
	{
		bm_info->bmiColors[i].rgbBlue = bm_info->bmiColors[i].rgbGreen = bm_info->bmiColors[i].rgbRed = i;
		bm_info->bmiColors[i].rgbReserved = 0;
	}

	SetDIBitsToDevice(hDC, 0, 0, DISPLAY_COLS, DISPLAY_ROWS, 0, 0,
		0, /* first scan line */
		DISPLAY_ROWS, /* number of scan lines */
		DisplayImage, bm_info, DIB_RGB_COLORS);
	ReleaseDC(MainWnd, hDC);
	EndPaint(MainWnd, &Painter);

	free(DisplayImage);
	free(bm_info);
}


DWORD WINAPI RegionGrow(LPVOID lpParam)
{
	//oneRegionGrowActive = 1;
	HDC hDC;
	int	r2, c2;
	int	queue[MAX_QUEUE], qh, qt;
	int	average, total, xtot, ytot;
	int xcen, ycen;
	unsigned char	*labels;
	int		count;
	int paint_over_label = 0;
	int new_label = 255;
	PMYDATA pData = (PMYDATA)lpParam;

	xtot = pData->c;
	ytot = pData->r;
	xcen = pData->c;
	ycen = pData->r;
	labels = (unsigned char *)calloc(pData->ROWS1*pData->COLS1, 1);

	labels[pData->r*pData->COLS1 + pData->c] = new_label;
	hDC = GetDC(MainWnd);
	SetPixel(hDC, pData->c, pData->r, RGB(red, green, blue));
	ReleaseDC(MainWnd, hDC);
	average = total = (int)pData->image[pData->r*pData->COLS1 + pData->c];
	queue[0] = pData->r*pData->COLS1 + pData->c;
	qh = 1;	/* queue head */
	qt = 0;	/* queue tail */
	count = 1;

	while (qt != qh)
	{
		for (r2 = -1; r2 <= 1; r2++)
			for (c2 = -1; c2 <= 1; c2++)
			{
				if (r2 == 0 && c2 == 0)
					continue;
				if ((queue[qt] / pData->COLS1 + r2) < 0 || (queue[qt] / pData->COLS1 + r2) >= pData->ROWS1 || (queue[qt] % pData->COLS1 + c2) < 0 || (queue[qt] % pData->COLS1 + c2) >= pData->COLS1)
					continue;
				if (labels[(queue[qt] / pData->COLS1 + r2)*pData->COLS1 + queue[qt] % pData->COLS1 + c2] != paint_over_label)
					continue;
				if (abs((int)(pData->image[(queue[qt] / pData->COLS1 + r2)*pData->COLS1 + queue[qt] % pData->COLS1 + c2]) - average) > pData->intensity_thresh)
					continue;	
				if(sqrt((((queue[qt] / pData->COLS1 + r2) - ycen)*((queue[qt] / pData->COLS1 + r2) - ycen)) + (((queue[qt] % pData->COLS1 + c2) - xcen)*((queue[qt] % pData->COLS1 + c2) - xcen)))>pData->centroid_thresh)
					continue;

				labels[(queue[qt] / pData->COLS1 + r2)*pData->COLS1 + queue[qt] % pData->COLS1 + c2] = new_label;
				if (jCounter != tempJCounter)
					tempJCounter++;
				if (playModeActive == 1)
				{
					Sleep(1);
				}
				else
				{
					while (stepModeActive == 1 && jCounter == tempJCounter)
					{
						//do nothing
					}
				}
				hDC = GetDC(MainWnd);
				SetPixel(hDC, queue[qt] % pData->COLS1 + c2, queue[qt] / pData->COLS1 + r2, RGB(red, green, blue));
				ReleaseDC(MainWnd, hDC);
				total += pData->image[(queue[qt] / pData->COLS1 + r2)*pData->COLS1 + queue[qt] % pData->COLS1 + c2];
				ytot += (queue[qt] / pData->COLS1 + r2);
				xtot += (queue[qt] % pData->COLS1 + c2);
				count++;
				average = total / (count);
				xcen = xtot / (count);
				ycen = ytot / (count);
				queue[qh] = (queue[qt] / pData->COLS1 + r2)*pData->COLS1 + queue[qt] % pData->COLS1 + c2;
				qh = (qh + 1) % MAX_QUEUE;
				if (qh == qt)
				{
					printf("Max queue size exceeded\n");
					exit(0);
				}
			}
		qt = (qt + 1) % MAX_QUEUE;
	}
	//oneRegionGrowActive = 0;
}


