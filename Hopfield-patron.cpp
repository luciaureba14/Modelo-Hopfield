#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <fstream>
#include <ctime>

using namespace std;


// --- FUNCION PARA CARGAR LOS PATRONES DESDE UN ARCHIVO ---
void CargarPatrones(const string& f_patron, vector<int>& patron, int& N_calculado) {
    ifstream archivo(f_patron);
    
    // Comprobamos si el archivo existe
    if (!archivo.is_open()) {
        cerr << "Error al abrir el archivo: " << f_patron << endl;
        return;
    }

    string linea;

    // Leemos el archivo línea por línea
    while (getline(archivo, linea)) { 
        
        for (int i = 0; i < linea.length(); i++) {
                char c = linea[i]; 
                
                //Convertimos los caracteres en números
                if (c == '1') {
                    patron.push_back(1);
                } else if (c == '0') {
                    patron.push_back(0);
                }
            }
        }
    N_calculado = patron.size(); // Actualizamos el tamaño N
    cout << "El valor de n es: " << sqrt(N_calculado) << endl;
    archivo.close();
}


// --- FUNCIÓN PARA RELLENAR CON RUIDO ALEATORIO TOTAL ---
void InicializarAleatorio(vector<int>& s, int N, mt19937& gen) {
    uniform_real_distribution<> dis(0.0, 1.0);
    for (int i = 0; i < N; i++) {
        double probabilidad = dis(gen); // Generamos un número aleatorio entre 0 y 1

        if (probabilidad < 0.5) {
            s[i] = 0; // El bit será 0
        } 
        else {
            s[i] = 1; // El bit será 1
        }
    } 
    cout << "Red inicializada con estado aleatorio." << endl;
}

// --- FUNCIÓN PARA COGER UN PATRÓN Y MODIFICARLO ---
void InicializarConPatronRuidoso(vector<int>& s, int N, const vector<int>& patron, double nivel_ruido, mt19937& gen) {
    uniform_real_distribution<> dis(0.0, 1.0);
    
    for (int i = 0; i < N; i++) {
        if (dis(gen) < nivel_ruido) {
            // Invertimos el bit
            s[i] = 1 - patron[i];
        } else {
            // Mantenemos el bit original
            s[i] = patron[i];
        }
    }
    cout << "Red inicializada con el patron original modificado al " << nivel_ruido * 100 << "%" << endl;
}


//-- REGLA DE APRENDIZAJE DE HEBB ---
void Regla_Hebb(int N,  double& a, vector<vector<double>>& w, vector<double>& theta, const vector<int>& patron) {
    
    int P = 1; // Solo un patrón
    
    // Calculamos a 
    double suma_a = 0;
        
    for (int i = 0; i < N; i++) suma_a += patron[i];
    
    a = suma_a / (double)(N * P);

    // Creamos la matriz de pesos (NxN) y el vector de umbrales (N) y los inicializamos a cero
    
    w.assign(N, vector<double>(N, 0.0));
    theta.assign(N, 0.0);
    
    // Calculamos el factor de normalización que va delante de la sumatoria
    double factor = 1.0 / (a * (1.0 - a) * N);

    // Calculamos los pesos de cada neurona con el resto de neuronas, esto nos da la matriz Wij
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (i != j){
            for (int mu = 0; mu < P; mu++) {
                w[i][j] += (patron[i] - a) * (patron[j] - a);
            }
            w[i][j] *= factor;
        }
        }
    }

    // Calculamos los Umbrales de cada neurona
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) theta[i] += w[i][j];
        theta[i] *= 0.5;
    }
}
 
//--- FUNCION PARA CALCULAR EL SOLAPAMIENTO ENTRE DOS PATRONES---
double CalcularSolapamiento(int N, double a, const vector<int>& s, const vector<int>& patron) {
    double suma = 0.0;

    for (int i = 0; i < N; i++) {
        // s[i] es la configuración actual de la red
        suma += (patron[i] - a) * (s[i]- 0.5);
    }

    // El factor de normalización es 1 / (a * (1 - a) * N)
    double factor = 1.0 / (a * (1.0 - a) * N);
    
    return factor * suma;
}


int main() {


    // Parámetros de simulación
    const double T = 0.02; // Fijamos la temperatura 
    const int Pasos_MC = 21; 
    
    int N = 0;
    vector<int> patron;
    
    // Cargamos los patrones
    CargarPatrones("minion.txt", patron, N);
    if (N == 0) cout << "No se han cargado los patrones correctamente" << endl;

    // --- Declaramos las variables para la red de Hopfield
    vector<int> s(N);
    vector<vector<double>> w;
    vector<double> theta;
    double a;

    mt19937 gen(time(0));
    uniform_real_distribution<> dis_real(0.0, 1.0);
    uniform_int_distribution<> dis_int(0, N - 1);
    
    // Abrimos los archivos para guardar las configuraciones de la red en cada paso Monte Carlo asi como el solapamiento
    ofstream f_Hopfield("configuraciones_MC.txt");
    ofstream f_solapamiento("solap_minion_02.txt");

   // Usamos la regla de Hebb para calcular los pesos y umbrales a partir de los patrones 
    Regla_Hebb(N, a, w, theta, patron);


   // Inicializamos el estado de la red
   
     InicializarAleatorio(s, N, gen);
   //InicializarConPatronRuidoso(s, N, patron, 0.3, gen); // El nivel de ruido es del 30%

    // Guardamos la configuración inicial de la red
    for(int i = 0; i < N; i++) {
            f_Hopfield << s[i] << " ";
        }
        f_Hopfield << "\n"; // Rellenamos una fila de s por cada paso monte carlo
        
    // Calculamos el solapamiento inicial con el patrón
    double solapamiento = CalcularSolapamiento(N, a, s, patron);
    f_solapamiento << 0 << " " << solapamiento << "\n";



    
    //-----BUCLE MONTE CARLO-----

    for(int mcs = 1; mcs < Pasos_MC; mcs++) {
        
        
        for(int n = 0; n < N; n++) {

            // Elegimos una neurona al azar
            int i = dis_int(gen);

            // Calculamos h_i (sumatoria de w_ij * s_j para j=1 a N)
            double h_i = 0.0; // Variable h para la neurona i
            for (int m = 0; m < N; m++) {
                h_i += w[i][m] * s[m];
            }
        
            // Calculamos Delta_E para esta neurona i
            double Delta_E = (2.0 * s[i] - 1.0) * (h_i - theta[i]);

            //Algoritmo de Metropolis
            if (Delta_E <= 0 || dis_real(gen) < exp(-Delta_E / T)) {
                s[i] = 1 - s[i]; // Cambiamos el estado de la neurona i
            }
    }
    
     // Calculamos el solpamiento
    double solapamiento = CalcularSolapamiento(N, a, s, patron);


     // Guardamos la configuracion de la red en un paso montecarlo
        
     for(int i = 0; i < N; i++) {
            f_Hopfield << s[i] << " ";
        }
        f_Hopfield << "\n"; // Un salto de línea al final de cada configuración completa (rellenamos una fila de s por cada paso monte carlo)
         
    
    // Guardamos el solapamiento en cada paso Monte Carlo junto con el tiempo
    f_solapamiento << mcs << " " << solapamiento << "\n";


}
        

    f_Hopfield.close();
    cout << "Simulacion terminada" << endl;

    return 0;
}