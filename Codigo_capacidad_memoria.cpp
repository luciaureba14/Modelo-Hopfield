#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <fstream>
#include <ctime>
#include <string>

using namespace std;

// ---FUNCION PARA GENERAR P PATRONES DE N NEURONAS ALEATORIOS---
void GenerarPatrones (int P, int N, vector<vector<int>>& patrones)
{
    mt19937 gen(time(0));
    uniform_real_distribution<> dis(0.0, 1.0);
    
    // Inicializamos la matriz de patrones con ceros
    patrones.assign(P, vector<int>(N, 0));
    
    for (int mu = 0; mu < P; mu++) {
        for (int i = 0; i < N; i++) {
            if(dis(gen) < 0.5) {
                patrones[mu][i] = 1;
            } else {
                patrones[mu][i] = 0;
            }
        }
    }
}

// --- REGLA DE APRENDIZAJE DE HEBB (PARA MÚLTIPLES PATRONES) ---
void Regla_Hebb(int N, double& a, vector<vector<double>>& w, vector<double>& theta, const vector<vector<int>>& patrones) {
    int P = patrones.size();
    
    // Calculamos la actividad media a de todos los patrones
    double suma_a = 0;
    for (int mu = 0; mu < P; mu++) {
        for (int i = 0; i < N; i++) {
            suma_a += patrones[mu][i];
        }
    }
    a = suma_a / (double)(N * P);

    // Calculamos los pesos y umbrales
    w.assign(N, vector<double>(N, 0.0));
    theta.assign(N, 0.0);
    
    
    double factor = 1.0 / (a * (1.0 - a) * N);

    // Calculamos la matriz de pesos sinápticos Wij
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (i != j) {
                for (int mu = 0; mu < P; mu++) {
                    w[i][j] += (patrones[mu][i] - a) * (patrones[mu][j] - a);
                }
                w[i][j] *= factor;
            }
        }
    }

    // Calculamos los Umbrales theta_i
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            theta[i] += w[i][j];
        }
        theta[i] *= 0.5;
    }
}

// --- CÁLCULO DEL SOLAPAMIENTO CON EL PATRÓN MU ---
double CalcularSolapamiento(int N, double a, const vector<int>& s, const vector<vector<int>>& patrones, int mu) {
    double suma = 0.0;
    for (int i = 0; i < N; i++) {
        suma += (patrones[mu][i] - a) * (s[i] - 0.5);
    }
    double factor = 1.0 / (a * (1.0 - a) * N);
    return factor * suma;
}

// --- INICIALIZACIÓN CON PATRÓN RUIDOSO ---
void InicializarConPatronRuidoso(vector<int>& s, int N, const vector<vector<int>>& patrones, int mu, double nivel_ruido, mt19937& gen) {
    uniform_real_distribution<> dis(0.0, 1.0);
    for (int i = 0; i < N; i++) {
        if (dis(gen) < nivel_ruido) {
            s[i] = 1 - patrones[mu][i]; // Invertimos el bit
        } else {
            s[i] = patrones[mu][i];
        }
    }
    
}

// --- INICIALIZACIÓN CON PATRÓN ALEATORIO ---
void InicializarPatronAleatorio(vector<int>& s, int N,  mt19937& gen) {
    uniform_real_distribution<> dis(0.0, 1.0);
    for (int i = 0; i < N; i++) {
        if (dis(gen) < 0.5) {
            s[i] = 1;
        } else {
            s[i] = 0;
        }
    }
    cout << "Red inicializada con patron aleatorio" << endl;
}

int main() {
    // Definimos los parámetros    
    const int N = 400;         
    const double T = 0.0001;       
    const int Pasos_MC = 100;
    mt19937 gen(time(0));
    uniform_real_distribution<> dis_real(0.0, 1.0);
    uniform_int_distribution<> dis_int(0, N - 1);


    
    ofstream f_grafica("datos_memoria_70_ruido30_2.txt");

   
    
    for (int P = 1; P <=70; P++) {
        
        vector<vector<int>> patrones;
        GenerarPatrones(P, N, patrones); // Crea P patrones
        
        double a;
        vector<vector<double>> w;
        vector<double> theta;
        Regla_Hebb(N, a, w, theta, patrones); // Aplicamos la regla de Hebb   

        int exitos_P = 0;

        // Probamos cada uno de los P patrones almacenados
        for (int mu = 0; mu < P; mu++) {
            
            // Inicializamos la red 
            vector<int> s(N);
            // InicializarPatronAleatorio(s, N, gen); 
            InicializarConPatronRuidoso(s, N, patrones, mu, 0.3, gen); 
            double suma_m = 0;

            // Bucle de tiempo de Monte Carlo
            for (int mcs = 0; mcs < Pasos_MC; mcs++) {
        
                for (int n = 0; n < N; n++) {
                    int i =dis_int(gen);

                    // Calculamos el campo local h_i para la neurona i
                    double h_i = 0.0;
                    for (int j = 0; j < N; j++) h_i += w[i][j] * s[j];

                    double Delta_E = (2.0 * s[i] - 1.0) * (h_i - theta[i]);
                    
                    
                    if (Delta_E <= 0 || dis_real(gen) < exp(-Delta_E / T) ) {
                        s[i] = 1 - s[i];
                    }
                }

                // Nos saltamos los 10 primeros pasos para que la red se estabilice antes de medir el solapamiento
                if (mcs >= 10) {
                    suma_m += CalcularSolapamiento(N, a, s, patrones, mu);
                }
            }

            // Hacemos el promedio temporal del solapamiento
            double m_final = suma_m / 90.0;

            
            if (abs(m_final) > 0.75) {
                exitos_P++;
            }
        }

        //  Calculamos alpha que es el número de patrones almacenados por neurona
        double alpha= 1.0*  P / N;
        double fraccion_exito = (double)exitos_P / P;


        f_grafica << P << " " << fraccion_exito << " " << alpha << endl;

        //Imprimimos en pantalla los parámetros calculados para cada P
        cout << "P: " << P << " | Exitos: " << exitos_P << " | Fraccion: " << fraccion_exito << endl;
        
    }

    f_grafica.close();
    cout << "Simulacion terminada, datos guardados en 'datos_memoria_70_ruido30_2.txt' ." << endl;
    return 0;
}
