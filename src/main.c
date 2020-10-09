// Read in voltage and and temperature data from battery
// Compare values against acceptable range
// On fault
    // Send high signal on some line(s)
    // Swap power source controlling low-power circuit
    // Trigger backup battery to run BMS instead of solar

#define VOLT_N 32
#define TEMP_N 32

#define VOLT_MIN 100
#define VOLT_MAX 240

#define VOLT_AVG_MIN 120
#define VOLT_AVG_MAX 220

#define TEMP_MIN 50
#define TEMP_MAX 85

#define TEMP_AVG_MIN 55
#define TEMP_AVG_MAX 80

enum FaultType
{
    None,
    Volt,
    Temp,
    Unknown
};

static const char* TypeLabels[] =
{
    "None",
    "Volt",
    "Temp",
    "Unknown"
};

struct FaultData
{
    enum FaultType type;
    char index;     // -1 if 'average' reading
    float value;
};

struct MetaData
{
    enum FaultType type;
    char n;
    float min;
    float max;
    float avgMin;
    float avgMax;
};

void updateVolts(char* volts)
{
    // Read from input
}

void updateTemps(char* temps)
{
    // Read from input
}

char checkValues(
    const float* data,
    const struct MetaData* meta,
    struct FaultData* fd)
{
    float sum = 0.0f;
    float val;
    
    // Check individual values
    for (char i = 0; i < meta->n; i++)
    {
        val = data[i];
        if (val < meta->min || val > meta->max)
        {
            fd->type = meta->type;
            fd->index = i;
            fd->value = val;
            return 1;
        }
        sum += val;
    }

    // Check average value
    const float avg = sum / meta->n;
    if (avg < meta->avgMin || avg > meta->avgMax)
    {
        fd->type = meta->type;
        fd->index = -1;
        fd->value = avg;
        return 1;
    }

    return 0;
}

void fault(struct FaultData* fd)
{
    printf("FAULT DETECTED\nType: %s\nIndex: %d\nValue: %f\n",
            TypeLabels[fd->type],
            fd->index,
            fd->value);

    // Then cut off power and stuff like that
}

int main(void)
{
    // Store most recent values from each reading
    float volts[VOLT_N];
    float temps[TEMP_N];

    // Track metadata for each data type
    struct MetaData voltMeta =
    {
        Volt,
        VOLT_N,
        VOLT_MIN,
        VOLT_MAX,
        VOLT_AVG_MIN,
        VOLT_AVG_MAX
    };
    struct MetaData tempMeta =
    {
        Temp,
        TEMP_N,
        TEMP_MIN,
        TEMP_MAX,
        TEMP_AVG_MIN,
        TEMP_AVG_MAX
    };

    // Track the state of the system
    struct FaultData fd;

    while (1)
    {
        // Poll inputs
        updateVolts(volts);
        updateTemps(temps);

        if (checkValues(volts, &voltMeta, &fd) ||
            checkValues(temps, &tempMeta, &fd))
        {
            fault(&fd);
        }
    }
}
