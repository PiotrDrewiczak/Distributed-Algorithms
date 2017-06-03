#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// ZMIENNE GLOBALNE

int pack_size = 0, pack_position = 0; // Rozmiar paczki i posycja w bufore (paczce) - wazne przy pakowaniu/rozpokowywaniu
char *pack_buffer;                    // paczka danych do wyslania/odebrania

int degree = 0;             // stopien wielomianu
float *coeffs;              // wspolczynniki wielomianu
float interval[2] = {0, 0}; // przedzial calkowania
int integration = 0;        // liczba punktow podzialu

double evaluate_f_of_x(double x)
{
    double sum = 0;
    for (int i = 0; i < degree + 1; i++)
    {
        sum += coeffs[i] * pow(x, i);	
    }
    return sum;
}

double integrate(double a, double b, int m)
{
    double x;
    double f_of_x;
    double h = (b - a) / m;
    double integral = 0.0;
    for (int i = 0; i <= m; i++)
    {
        x = a + i * h;
        f_of_x = evaluate_f_of_x(x);
        if ((i == 0) || (i == m))
            integral += 0.5 * f_of_x;
        else
            integral += f_of_x;
    }
    integral *= h;
    return integral;
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 2) // Jezeli nie podano nazwy pliku
    {
        MPI_Finalize();
        return 1;
    }

    if (rank == 0)
    {
        FILE *file = fopen(argv[1], "r");
        if (file == NULL)
        {
            printf("Nie mozna otworzyc pliku %s!\n", argv[1]);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        // czytanie pliku na masterze

        fscanf(file, "%*s %i", &degree);         // %*s - ignoruj stringa, wczytaj int'a
        coeffs = malloc((degree+1) * sizeof(float)); // alokacja pamieci dla wspolczynnikow

        fscanf(file, "%*s");
        for (int i = 0; i < degree + 1; i++)
        {
            fscanf(file, "%f", &coeffs[i]);
        }
        fscanf(file, "%*s");
        for (int i = 0; i < 2; i++)
        {
            fscanf(file, "%f", &interval[i]);
        }
        fscanf(file, "%*s %i", &integration);

        // liczenie rozmiaru paczki
        MPI_Pack_size(2, MPI_INT, MPI_COMM_WORLD, &pack_size); // Zawsze beda 2 int'y
        int tmp_pack_size = pack_size;
        MPI_Pack_size(degree + 3, MPI_FLOAT, MPI_COMM_WORLD, &pack_size); // degree + 1 -> ilosc wspolczynnikow, + 2 -> przedzialy calkowania, czyli w sume degree + 3 floatow
        pack_size += tmp_pack_size;

        pack_buffer = malloc(pack_size * sizeof(char)); // alokacja pamieci dla bufora do wyslania
        // pakowanie w kolejnosci int, floaty, int
        MPI_Pack(&degree, 1, MPI_INT, pack_buffer, pack_size, &pack_position, MPI_COMM_WORLD);
        MPI_Pack(coeffs, degree + 1, MPI_FLOAT, pack_buffer, pack_size, &pack_position, MPI_COMM_WORLD);
        MPI_Pack(interval, 2, MPI_FLOAT, pack_buffer, pack_size, &pack_position, MPI_COMM_WORLD);
        MPI_Pack(&integration, 1, MPI_INT, pack_buffer, pack_size, &pack_position, MPI_COMM_WORLD);

        // Wyslanie pack_size i poten bufor
        MPI_Bcast(&pack_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(pack_buffer, pack_size, MPI_PACKED, 0, MPI_COMM_WORLD);

        free(pack_buffer);
    }
    else
    {
        // Odbieranie danych od mastera przez pozostalych w tej samej kolejnosci jak wyzej
        MPI_Bcast(&pack_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
        pack_buffer = malloc(pack_size * sizeof(char));
        MPI_Bcast(pack_buffer, pack_size, MPI_PACKED, 0, MPI_COMM_WORLD);

        // Odpakowywanie
        MPI_Unpack(pack_buffer, pack_size, &pack_position, &degree, 1, MPI_INT, MPI_COMM_WORLD);

        coeffs = malloc((degree+1) * sizeof(float));
        MPI_Unpack(pack_buffer, pack_size, &pack_position, coeffs, degree + 1, MPI_FLOAT, MPI_COMM_WORLD);
        MPI_Unpack(pack_buffer, pack_size, &pack_position, interval, 2, MPI_FLOAT, MPI_COMM_WORLD);
        MPI_Unpack(pack_buffer, pack_size, &pack_position, &integration, 1, MPI_INT, MPI_COMM_WORLD);

        free(pack_buffer);
    }

    // Dekompozycja z pliku pdf
    int p = integration / size;
    int r = integration % size;
    int my_number_of_subintervals;
    int my_first_midpoint;
    if (rank < r)
    {
        my_number_of_subintervals = p + 1;
        my_first_midpoint = rank * (p + 1);
    }
    else
    {
        my_number_of_subintervals = p;
        my_first_midpoint = r * (p + 1) + (rank - r) * p;
    }
    double a = interval[0] + ((interval[1] - interval[0]) / integration * (my_first_midpoint)); // dolna granica calkowania dla procesu
    double b = interval[0] + ((interval[1] - interval[0]) / integration * (my_first_midpoint + my_number_of_subintervals)); // gorna granica calkowania dla procesu

    double result = integrate(a, b, my_number_of_subintervals); // wynik czastkowy (inny na kazdym procesie)
    double total_result = 0;
    MPI_Reduce(&result, &total_result, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0)
    {
        // Obliczanie analityczne calki
        double integral_a = 0, integral_b = 0; // wartosc calki dla dolnej i gornej granicy calkowania
        for (int i = 0; i < degree + 1; i++) 
        {
            integral_a += coeffs[i] * pow(interval[0], i + 1) / (i + 1);
            integral_b += coeffs[i] * pow(interval[1], i + 1) / (i + 1);
        }

        // Wyswietlenie wyniku numerycznego i analitycznego
        printf("Result: %.9f\n", total_result);
        printf("Master result: %.9f\n", integral_b - integral_a);
    }

    free(coeffs);

    MPI_Finalize();
    return 0;
}