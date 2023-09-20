#include <assert.h>
#include <stdlib.h>
#include <glib.h>
#include <poppler.h>
#include <poppler-document.h>
#include <poppler-page.h>
#include <cairo.h>
#include <cairo-svg.h>
#include <stdio.h>
#include <string.h>
#include <cairo-pdf.h>
#include <math.h>
#include <stdio.h>

#define MAX_PARAM_LENGTH 256
#define MAX_PAGE_INDEX_LENGTH 16

enum {
	SUCCESS = 0,
	ERROR_UNKNOWN_TAG = -10,
	ERROR_CAIRO_STATUS = -11,
	ERROR_LACK_OF_TAG_INFORMATION = -12,
	ERROR_CANNNOT_OPEN_FILE = -13,
};

// Begin theft from ePDFview (Copyright (C) 2006 Emma's Software) under the GPL
gchar *getAbsoluteFileName(const gchar *fileName)
{
	gchar *absoluteFileName = NULL;
	if (g_path_is_absolute(fileName)) {
		absoluteFileName = g_strdup(fileName);
	}
	else {
		gchar *currentDir = g_get_current_dir();
		absoluteFileName = g_build_filename(currentDir, fileName, NULL);
		g_free(currentDir);
	}
	return absoluteFileName;
}
// End theft from ePDFview

int getPageIndex(const char* filePath) {
	char *p = NULL;
	char *end;
	char pageIndexStr[MAX_PAGE_INDEX_LENGTH+1];
	int i=0;
	long num;
	
	memset(pageIndexStr,0,sizeof(pageIndexStr));
	p = strstr(filePath,"#page=");
	if(p == NULL) return 0;
	strncpy(pageIndexStr,p+6,MAX_PAGE_INDEX_LENGTH);
	num = strtol(pageIndexStr,&end,10);
	if (*end != '\0') {
		return 0;	// エラーの場合は1ページ目とする
	}
	if(num < 1) {	// ページ数は1オリジン、pageIndexは0オリジンのため、1以上ない場合のエラー処理
		return 0;
	}
	return (int)(num-1);
}

int createOverlayPdf(const gchar *pdfParamsName , const char* outputFilename)
{
	char tag[MAX_PARAM_LENGTH+1];
	char value[MAX_PARAM_LENGTH+1];
	char filePath[MAX_PARAM_LENGTH+1];
	char s3FilePath[MAX_PARAM_LENGTH+1];
	FILE* fp;
	double pageWidth = 0;
	double pageHeight = 0;
	double originalWidth = 0;
	double originalHeight = 0;
	double scale = 0;
	int ret;
	cairo_surface_t *surfaceOutputFile;
	cairo_surface_t *surfaceLayer;
	cairo_t *cairoOutputFile;
	cairo_t *cairoLayer;
	PopplerPage *popplerPage;
	PopplerDocument *popplerDoc;
	int number = 0;
	int isBackground;
	double layerWidth = 0;
	double layerHeight = 0;
	double pdfWidth = 0;
	double pdfHeight = 0;
	double opacity = 0;
	double translateX = 0;
	double translateY = 0;
	double angle = 0;
	double rotateBaseX = 0;
	double rotateBaseY = 0;
	unsigned char hasFilePath = FALSE;
	unsigned char hasBackground = FALSE;
	unsigned char hasPDFSize = FALSE;
	unsigned char hasOpacity = FALSE;
	unsigned char hasTranslate = FALSE;
	unsigned char hasRotate = FALSE;
	unsigned char hasS3FilePath = FALSE;
	double pdfScale = 0;
	char *p = NULL;
	int pageIndex = 0;	

	memset(tag,0,sizeof(tag));
	memset(value,0,sizeof(value));
	memset(filePath,0,sizeof(filePath));
	memset(s3FilePath,0,sizeof(s3FilePath));
	fp = fopen(pdfParamsName,"r");

	while(fgets(tag,MAX_PARAM_LENGTH,fp) != NULL){
		if (strncmp(tag,"#",1) == 0){
			if(strcmp(tag,"#PDFParamsStart\n") == 0) {
				break;
			}
			if (fgets(value,MAX_PARAM_LENGTH,fp) == NULL) break;
			if(strcmp(tag,"#PageSize\n") == 0) {
				ret = sscanf(value,"%lf %lf",&pageWidth,&pageHeight);
			} else if(strcmp(tag,"#OriginalSize\n") == 0) {
				ret = sscanf(value,"%lf %lf",&originalWidth, &originalHeight);
			} else if(strcmp(tag,"#Scale\n") == 0) {
				ret = sscanf(value,"%lf",&scale);
			} else {
				printf("Unknown Tag Appeared!\n");
				return ERROR_UNKNOWN_TAG;
			}
		}
	}

	surfaceOutputFile = cairo_pdf_surface_create(outputFilename, pageWidth, pageHeight);
	if (cairo_surface_status(surfaceOutputFile) !=  CAIRO_STATUS_SUCCESS) {	
		return ERROR_CAIRO_STATUS;
	}
	cairoOutputFile = cairo_create(surfaceOutputFile);

	while(fgets(tag,MAX_PARAM_LENGTH,fp) != NULL){
		if (strncmp(tag,"##",2) == 0){
			if (fgets(value,MAX_PARAM_LENGTH,fp) == NULL) break;
			if(strcmp(tag,"##Number\n") == 0) {
				ret = sscanf(value,"%d",&number);
			} else if(strcmp(tag,"##FilePath\n") == 0) {
				ret = sscanf(value,"%s",filePath);
				strcpy(filePath,value);
				p = strchr(filePath,'\n');
				if(p != NULL) (*p) = '\0';
				p = strstr(filePath,"#page=");
				if(p != NULL) {
					pageIndex = getPageIndex(filePath);
					*p = '\0';	// #以降を上書きする
				} else {
					pageIndex = 0;
				}
				hasFilePath = TRUE;
			} else if(strcmp(tag,"##Background\n") == 0) {
				ret = sscanf(value,"%d",&isBackground);
				hasBackground = TRUE;
			} else if(strcmp(tag,"##PDFSize\n") == 0) {
				ret = sscanf(value,"%lf %lf",&layerWidth,&layerHeight);
				hasPDFSize = TRUE;
			} else if(strcmp(tag,"##Opacity\n") == 0) {
				ret = sscanf(value,"%lf",&opacity);
				hasOpacity = TRUE;
			} else if(strcmp(tag,"##Translate\n") == 0) {
				ret = sscanf(value,"%lf %lf",&translateX,&translateY);
				hasTranslate = TRUE;
				translateX *= scale;
				translateY *= scale;
			} else if(strcmp(tag,"##Rotate\n") == 0) {
				ret = sscanf(value,"%lf %lf %lf",&angle,&rotateBaseX,&rotateBaseY);
				hasRotate = TRUE;
				rotateBaseX *= scale;
				rotateBaseY *= scale;
			} else if(strcmp(tag,"##S3FilePath\n") == 0) {
				ret = sscanf(value,"%s",s3FilePath);
				strcpy(s3FilePath,value);
				p = strchr(s3FilePath,'\n');
				if(p != NULL) (*p) = '\0';
				hasS3FilePath = TRUE;
			} else {
				printf("Unknown Tag Appeared!\n");
				return ERROR_UNKNOWN_TAG;
			}
		} else {
			if (strcmp(tag,"#PDFParamsStart\n") == 0) {
				// do nothing
			} else if(strcmp(tag,"#PDFParamsEnd\n") == 0) {
				if (!hasFilePath || !hasBackground || !hasOpacity) return ERROR_LACK_OF_TAG_INFORMATION;
				gchar *absoluteFileName = getAbsoluteFileName(filePath);
				gchar *filename_uri = g_filename_to_uri(absoluteFileName, NULL, NULL);
				popplerDoc = poppler_document_new_from_file(filename_uri, NULL, NULL);
				g_free(absoluteFileName);
				g_free(filename_uri);
				if (popplerDoc == NULL) {
					fprintf(stderr, "Unable to open file : %s\n",filePath);
					hasFilePath = FALSE;
					hasBackground = FALSE;
					hasPDFSize = FALSE;
					hasOpacity = FALSE;
					hasTranslate = FALSE;
					hasRotate = FALSE;
					hasS3FilePath = FALSE;
					continue;
				}
				popplerPage = poppler_document_get_page(popplerDoc, pageIndex);
				poppler_page_get_size (popplerPage, &pdfWidth, &pdfHeight);

				if (hasBackground) {
					if (pdfWidth * pageHeight <= pdfHeight * pageWidth) {
						pdfScale = pageHeight / pdfHeight;
					} else {
						pdfScale = pageWidth / pdfWidth;
					}
				} else {
					pdfScale = 1;
				}
				
				surfaceLayer = cairo_pdf_surface_create(NULL, pageWidth, pageHeight);
				if (cairo_surface_status(surfaceLayer) !=  CAIRO_STATUS_SUCCESS) {
					return ERROR_CAIRO_STATUS;
				}
				cairoLayer = cairo_create(surfaceLayer);
				
				if (hasTranslate == TRUE) {
					cairo_translate( cairoLayer, translateX, translateY);
				}

				if (hasRotate == TRUE) {
					cairo_translate( cairoLayer, rotateBaseX, rotateBaseY );
					cairo_rotate( cairoLayer, angle*M_PI/180 );
					cairo_translate( cairoLayer, -rotateBaseX, -rotateBaseY );
				}
				
				if ( pdfScale != 1 ) {
					cairo_scale(cairoLayer,pdfScale,pdfScale);
				}

				if (isBackground == TRUE) {
					cairo_set_source_rgb( cairoLayer, 1, 1, 1 );
					cairo_rectangle( cairoLayer, 0, 0, pdfWidth, pdfHeight );
					cairo_fill( cairoLayer );
				}

				poppler_page_render_for_printing(popplerPage, cairoLayer);
				cairo_set_source_surface(cairoOutputFile, surfaceLayer, 0.0, 0.0);

				cairo_paint_with_alpha (cairoOutputFile, opacity);
				cairo_paint(cairoLayer);

				cairo_destroy(cairoLayer);
				cairo_surface_destroy(surfaceLayer);

				g_object_unref(popplerPage);
				g_object_unref(popplerDoc);

				hasFilePath = FALSE;
				hasBackground = FALSE;
				hasPDFSize = FALSE;
				hasOpacity = FALSE;
				hasTranslate = FALSE;
				hasRotate = FALSE;
				hasS3FilePath = FALSE;
			}
		}
	}

	cairo_destroy(cairoOutputFile);
	cairo_surface_destroy(surfaceOutputFile);

	fclose(fp);
	
	return SUCCESS;
}

int main(int argn, char *args[])
{
	// Poppler stuff
	PopplerDocument *pdffile;
	PopplerPage *page;
	int ret = 0;

	// Initialise the GType library
	g_type_init ();

	// Command line options
	GOptionContext *context;
	GError *err = NULL;
	static gint firstPage = 0;
	static gint lastPage = 0;
	GOptionEntry entries[] = {
		{ "first", 'f', 0, G_OPTION_ARG_INT, &firstPage, "First page", "<int>" },
		{ "last", 'l', 0, G_OPTION_ARG_INT, &lastPage, "Last page", "<int>" },
		{ NULL }
	};
	context = g_option_context_new("<in file.pdf> <out file.svg> [<page no>]");
	g_option_context_add_main_entries(context, entries, NULL);
	if (!g_option_context_parse(context, &argn, &args, &err)) {
		g_print("option parsing failed: %s\n", err->message);
		g_error_free(err);
		return -6;
	}
	g_option_context_free(context);

	if(argn == 3) {
		ret = createOverlayPdf(args[1],args[2]);
	} else {
		printf("Usage(additional): overlaypdf <in PDFParams.txt> <out file.pdf>\n");
		ret = -2;
	}
	return ret;
}
