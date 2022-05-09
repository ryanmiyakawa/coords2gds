/**
 * @file text2gds.cpp
 * @author Ryan Miyakawa (rhmiyakawa@lbl.gov)
 * @brief 
 * @version 0.1
 * @date 2022-05-03
 * 
 * @copyright Copyright (c) 2022
 * 
 * Usage: ./text2gds input.csv output.gds
 * 
 * Reads a comma separated description of polygons and writes to GDS.  
 * 
 * CSV should have coordinate pairs separated by commas with one row per polygons: x1,y1,x2,y2 ...
 * Do not close boundaries; a redundant coordinate will be appended to end of list as per GDSII standard
 * 
 * Coordinates should not have whitespace.
 * 
 * As per GDSII standards, polygon boundaries should not intersect or cross.
 * https://www.iue.tuwien.ac.at/phd/minixhofer/node52.html
 * 
 */

#include <stdio.h>
#include <fstream>
#include <cmath>
#include <ctime>
#include <string.h>
#include <iostream>

using namespace std;


/**
 * @brief Defines GDS pre and postambles
 * 
 * @param outputFile 
 * @param gdsPost 
 * @param polyPre 
 * @param polyPost 
 * @param layerNumber 
 */
void initGDS(FILE *outputFile, unsigned char *gdsPost, unsigned char *polyPre, unsigned char *polyPost, int layerNumber)
{
    const int gdspreamble[102] = {0, 6, 0, 2, 0, 7, 0, 28, 1, 2, 230, 43, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0,
                            230, 43, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 10, 2, 6, 110, 111, 110, 97,
                            109, 101, 0, 20, 3, 5, 61, 104, 219, 139, 172, 113, 12, 180, 56, 109,
                            243, 127, 103, 94, 246, 236, 0, 28, 5, 2, 0, 114, 0, 4, 0, 17, 0, 13,
                            0, 22, 0, 56, 0, 114, 0, 4, 0, 17, 0, 13, 0, 22, 0, 56,
                            0, 10, 6, 6, 110, 111, 110, 97, 109, 101};

    const int gdspostamble[8] = {0, 4, 7, 0, 0, 4, 4, 0};
    const int polypreamble[16] = {0, 4, 8, 0, 0, 6, 13, 2, 0, layerNumber, 0, 6, 14, 2, 0, 0};
    const int polypostamble[4] = {0, 4, 17, 0};
    const int polyBlockFormat[4] = {0, 44, 16, 3};

    unsigned char gdsPre[102];
    for (int k = 0; k < 102; k++)
        gdsPre[k] = (unsigned char)gdspreamble[k];
    for (int k = 0; k < 8; k++)
        gdsPost[k] = (unsigned char)gdspostamble[k];
    for (int k = 0; k < 16; k++)
        polyPre[k] = (unsigned char)polypreamble[k];
    for (int k = 0; k < 4; k++)
        polyPost[k] = (unsigned char)polypostamble[k];
   

    fwrite(gdsPre, sizeof(char), 102, outputFile);
}

/**
 * @brief Computes polygon header
 * 
 * @param polyForm 
 * @param tokenCount 
 */
void getPolyForm(unsigned char *polyForm, int tokenCount){
    const int byteCt = 4 + 4*tokenCount;
     const int polyBlockFormat[4] = {byteCt >> 8, byteCt & 255, 16, 3};
     for (int k = 0; k < 4; k++)
        polyForm[k] = (unsigned char)polyBlockFormat[k];
}

/** 
 * @brief Encodes a 32-bit integer
 * 
 * @param aCoord 
 * @param cPart 
 */
void encode32(long aCoord, char *cPart)
{
    cPart[0] = (aCoord >> 24) & 255;
    cPart[1] = (aCoord >> 16) & 255;
    cPart[2] = (aCoord >> 8) & 255;
    cPart[3] = (aCoord)&255;
}

/**
 * @brief Enocodes polygon coordinates into a series of 32-bit integers
 * 
 * @param coords 
 * @param cCoords 
 * @param tokenCount 
 */
void encodePoly32(int *coords, char *cCoords, int tokenCount)
{
    char cPart[4];
    for (int k = 0; k < tokenCount; k++)
    {
        encode32(coords[k], cPart);
        for (int m = 0; m < 4; m++)
        {
            cCoords[k * 4 + m] = cPart[m];
        }
    }
}

/**
 * @brief Writes the GDS postamble and closes file
 * 
 * @param outputFile 
 * @param gdsPost 
 */
void renderGDS(FILE *outputFile, unsigned char *gdsPost)
{
    fwrite(gdsPost, sizeof(char), 8, outputFile);
    fclose(outputFile);
}

/**
 * @brief Enocodes polygon coordinates into a GDSII record
 * 
 * @param coords 
 * @param cCoords 
 * @param tokenCount 
 */
void exportPolygon(int *coords, unsigned char *polyPre, unsigned char *polyPost, unsigned char *polyForm, FILE *outputFile, int tokenCount)
{

    // Get polyform for this coordinate count:
    getPolyForm(polyForm, tokenCount);

    int polyByteCount = tokenCount * 4;

    char cCoords[polyByteCount];

    fwrite(polyPre, sizeof(char), 16, outputFile);
    fwrite(polyForm, sizeof(char), 4, outputFile);
    encodePoly32(coords, cCoords, tokenCount);
    fwrite(cCoords, sizeof(char), polyByteCount, outputFile);
    fwrite(polyPost, sizeof(char), 4, outputFile);

}


/**
 * @brief Main function.  See header comments for usage
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char **argv)
{
    clock_t begin = clock();

    char **argv_test;
    argv_test = argv + 1;
    
    char *inFile = argv[1];
    char *outFile = argv[2];
    char *flag = argv[3];

    bool echoCounts = false;

    if (argc < 3)
    {
        // printf("Please specify inputfile and output file! \n");
        // system("PAUSE");
        // return 0;

        inFile =  (char *) "gratingwriter_hologram_220508b.csv";
        outFile =   (char *) "out.gds";
    } else {
        inFile = argv[1];
        outFile = argv[2];
    }


    // Echo flag
    char echoFlag[] = "-echoCoords";
    if (argc == 4 && strcmp(flag, echoFlag) == 0){
        echoCounts = true;
    }


    FILE* fp = fopen(inFile, "r");
    FILE *outputFile = NULL;

    unsigned char gdsPost[8];
    unsigned char polyPre[16];
    unsigned char polyPost[4];
    unsigned char polyForm[4];

    const int layerNumber = 0;
    
    // Populate ambles
    if ((outputFile = fopen(outFile, "wb")) == NULL)
        printf("Cannot open file.\n");
    initGDS(outputFile, gdsPost, polyPre, polyPost, layerNumber);


    if (fp == NULL){
        printf("Cannot find file: %s\n", inFile);
        exit(EXIT_FAILURE);
    }

    char* line = NULL;
    size_t len = 0;

    int coords[1024];
    int tokenCount = 0;
    int coordCount = 0;
    int lineCount = 0;

    char * token;

    // Read lines
    while ((getline(&line, &len, fp)) != -1) {
        tokenCount = 0;
        lineCount++;


        // Tokenize integers
        token = strtok(line, ",");
        while( token != NULL ) {
            // printf( " %s\n", token ); //printing each token

            coords[tokenCount] = strtol(token, nullptr, 10);
            tokenCount++;

            token = strtok(NULL, ",");

            if (tokenCount > 512){
                break;
            }
        }

        // Add a final set of coordinates to close the boundary:
        coords[tokenCount] = coords[0];
        coords[tokenCount + 1] = coords[1];
        tokenCount += 2;


        if (tokenCount % 2 != 0){
            printf("ERROR: line %d does not have an even number of coordinates\n", lineCount);
            fclose(fp);
            if (line)
                free(line);

            return 0;
        } else {
            if (echoCounts){
                printf("Writing shape %d with %d coordinates\n", lineCount, tokenCount/2);
            }
        }

        if (tokenCount > 512){
            printf("ERROR: Shape %d has %d coordinates. MAX = 256\n", lineCount, tokenCount/2);

            fclose(fp);
            if (line)
                free(line);

            return 0;
        }

        // printf("Boundary %d has %d coordinate pairs (vertices)\n", lineCount, tokenCount/2);
        exportPolygon(coords, polyPre, polyPost, polyForm, outputFile, tokenCount);       

        const int lineEcho = 500;
        if (lineCount % lineEcho == 0 && lineCount > 0){
            printf("Parsed and processed %d lines\n", lineCount);
        }
    }

    fclose(fp);
    if (line)
        free(line);

    renderGDS(outputFile, gdsPost);


    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;

    printf("Shape translation finished in %0.3fs\n", elapsed_secs);
    printf("Successfully wrote %d shapes to %s!!\n", lineCount, outFile);

    return 0;
}
