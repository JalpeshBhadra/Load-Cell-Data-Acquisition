#include <MEGSV86w32.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

/* Function Declaration */
int InitSensor(int ComNo, int Chan, double ElecFSval, double PhysFSval);
char* TimeStamp();


short int i,ComNo =  7;
int count ;
char ErrTxt[ERRTEXT_SIZE] ;
double *Readbuffer;
int NumObj, MaxNumberofData = 16000;
double DataRate = 100 ; // in Hertz
double ScaleFactor[8];
FILE *logFile ;


int main()
{
    // File Opening
    logFile = fopen("logfile.txt","w");
    if (logFile ==NULL)
    {
        printf("File Opening Error\n");
        exit(1);
    }

    // Allocate memory for read Buffer
    Readbuffer = (double*)malloc(MaxNumberofData * sizeof(double));
    if (Readbuffer==NULL)
    {
        printf("Memory Allocation Error\n");
        return 0;
    }
    // Opening Resource for Communication
    int retcode = GSV86actExt(ComNo);
    if (retcode==GSV_ERROR)
    {
		GSV86getLastErrorText(ComNo, ErrTxt);
		printf("Activation failed. Reason:\n%s", ErrTxt);
		return 0;
    }
    //printf("Activation Successful\n\n");
    // Stopping Transmission for Settings
    GSV86stopTX(ComNo);
    // Sensor Initialization
    if (InitSensor(ComNo, 8, 3.5, 10000.0) != GSV_OK)
	{
		GSV86getLastErrorText(ComNo, ErrTxt);
		printf("Sensor Initialization Failed. Reason:\n%s", ErrTxt);
	}
	else
	{
	    printf("InitSensor successful");
	}
    //printf("The Serial Number of the Device: %i\n",GSV86getSerialNo(ComNo));
    // Clearing Buffers
    GSV86clearDeviceBuf(ComNo);
    GSV86clearDLLbuffer(ComNo);

    // Set Data Frequency 100Hz
    GSV86setFrequency(ComNo,DataRate);
    // Enable Analog Filter to Mode Auto
    GSV86setModeAfilterAuto(ComNo,1);
    // Setting Zero Error (Make Sure No load on the Plate)
    GSV86setZero(ComNo,0);
    Sleep(5000);

    // Starting Communication
    GSV86startTX(ComNo);
    NumObj = GSV86getValObjectInfo(ComNo, &ScaleFactor[0], NULL, NULL);
    while(!kbhit())
    {
        if (GSV86received(ComNo,1) > 8)
        {
            retcode = GSV86readMultiple(ComNo,0,Readbuffer,8,&count,NULL);
            if (retcode == GSV_TRUE)
            {
                printf("%s\t",TimeStamp());
                fprintf(logFile,"%s,",TimeStamp());
                for(i=0;i<8;i++)
                {
                    printf("%f\t",*(Readbuffer+i)*ScaleFactor[i]);
                    fprintf(logFile,"%f,",*(Readbuffer+i)*ScaleFactor[i]);
                }
                printf("\n");
                fprintf(logFile,"\n");
            }
            else if (retcode==GSV_OK)
            {
                Sleep(5);
            }
            else if (retcode == GSV_ERROR)
            {
                GSV86getLastErrorText(ComNo, ErrTxt);
                printf("GSV8readMultiple failed. Reason:\n%s", ErrTxt);

            }
        }
    }



    // Release the Resources
    fclose(logFile);
    free(Readbuffer);
    GSV86release(ComNo);

    //printf("\nExiting\n\n");

    printf("\nExiting\n\n");
    getch();
    return 0;
}

/* INIT Sensor    Not Sure What it Does Recommended By User manual*/
int InitSensor(int ComNo, int Chan, double ElecFSval, double PhysFSval)
{
	double InRange, ScaleDesired, ScaleAct;
	int typeDesired, typeAct, ret;
	char ErrTxt[256] = "Emty";

	if (ElecFSval <= 0 || PhysFSval <= 0)
	{
		return GSV_ERROR;
	}
	if (ElecFSval > 3.5)
	{
		typeDesired = 2;
	}
	else typeDesired = 1;
	typeAct = GSV86getInTypeRange(ComNo, Chan, &InRange);
		if (typeAct == GSV_ERROR)
		{
			ret = GSV86getLastErrorText(ComNo, ErrTxt);
			printf("%s", ErrTxt);
			return ret;
	}
		if (typeAct != typeDesired)
		{
			ret = GSV86setInType(ComNo, Chan, typeDesired);
			if (ret == GSV_ERROR)
			{
				ret = GSV86getLastErrorText(ComNo, ErrTxt);
				printf("%s", ErrTxt);
				return ret;
			}
			typeAct = GSV86getInTypeRange(ComNo, Chan, &InRange);
		}
		ScaleDesired = (PhysFSval*InRange) / ElecFSval;
		ret = GSV86readUserScale(ComNo, Chan, &ScaleAct);
		if (ret == GSV_ERROR)
		{
			ret = GSV86getLastErrorText(ComNo, ErrTxt);
			printf("%s", ErrTxt);
			return ret;
		}
		if ((ScaleAct + (ScaleAct / 1E5)) > ScaleDesired && (ScaleAct - (ScaleAct / 1E5)) < ScaleDesired)
		{
			ret = GSV_OK;
		}
		else
		{
			ret = GSV86writeUserScale(ComNo, Chan, ScaleDesired);
			if (ret == GSV_ERROR)
			{
				ret = GSV86getLastErrorText(ComNo, ErrTxt);
				printf("%s", ErrTxt);
			}
		}
		return ret;
    }

char* TimeStamp()
{
    static char buffer[20];
    SYSTEMTIME lt;
    GetLocalTime(&lt);
    sprintf(buffer,"%02d:%02d:%02d.%03d", lt.wHour, lt.wMinute,lt.wSecond,lt.wMilliseconds);
    //printf("The Value of time is: %s\n",buffer);
    return buffer ;
}
