#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <fstream>
#include <ctime>
#include <string>

using namespace std;

// --- FUNCION PARA CARGAR UN PATRÓN POR REFERENCIA ---
void CargarPatron(const string& nombre_archivo, vector<int>& patron, int& N_calculado) {
    ifstream archivo(nombre_archivo);
    patron.clear(); // Limpiamos el vector antes de empezar

    // Comprobamos si el archivo existe
    if (!archivo.is_open()) {
        cerr << "Error al abrir el archivo: " << nombre_archivo << endl;
        return; 
    }

    char c; // Convertimos los caracteres en numeros
    while (archivo >> c) {
        if (c == '1') {
            patron.push_back(1);
        } else if (c == '0') {
            patron.push_back(0);
        }
    }

    N_calculado = patron.size();
    cout<< "El valor de n es: " << sqrt(N_calculado) << endl;
    archivo.close();
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

    // Inicializamos pesos y umbrales
    w.assign(N, vector<double>(N, 0.0));
    theta.assign(N, 0.0);
    
    // Calculamos el factor de normalización
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

    // Calculamoslos Umbrales theta_i
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


//---- CÁLCULO DE LA MATRIZ DE CORRELACIÓN ENTRE LOS PATRONES ---    
double CalcularMatrizCorrelacion(int N, double a, const vector<vector<int>>& patrones, const vector<string>& nombres_archivos) {
    int P = patrones.size();
    if (P <= 1) return 0.0;

    double suma_fuera_diagonal = 0.0;
    int contador = 0;


    for (int mu = 0; mu < P; mu++) {
        for (int nu = mu + 1; nu < P; nu++) {
            int igual_neurona = 0;

            // Recorremos la matriz de cada patron
            for (int i = 0; i < N; i++) {
                // Si son iguales, contamos una coincidencia
                if (patrones[mu][i] == patrones[nu][i]) {
                    igual_neurona++;
                }
            }

            // La correlación es simplemente la fracción de neuronas que son iguales
            double correlacion = (double)igual_neurona / (double)N;

            // Mostramos la correlación entre los patrones mu y nu
            cout << " -> " << nombres_archivos[mu] << " vs " << nombres_archivos[nu] 
                 << "  |  Coincidencia = " << correlacion * 100.0 << "% (" << correlacion << ")" << endl;

            suma_fuera_diagonal += correlacion;
            contador++;
        }
    }
   
    return suma_fuera_diagonal / (double)contador;
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
    cout << "Red inicializada con patron " << mu << " (ruido: " << nivel_ruido * 100 << "%)" << endl;
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
    
    vector<string> archivos = {"p0.txt","p1.txt"}; 
    vector<vector<int>> patrones;
    int N = 0;

    // --- CARGA DE DATOS ---

    for (int i = 0; i < archivos.size(); i++) {
    string nombre = archivos[i]; // Leemos el nombre del patrón
    
    vector<int> patron_i;
    int n_i = 0;
    CargarPatron(nombre, patron_i, n_i);

        if (n_i == 0) {
            cerr << "Error al cargar el patrón desde " << nombre << ". Saliendo..." << endl;
            return 1; // 
        }
        patrones.push_back(patron_i); // Añadimos el patrón cargado a la lista de patrones
        N = n_i; // Asumimos que todos los patrones tienen el mismo tamaño 
    } 
    
    // --- VARIABLES DE LA RED ---
    vector<int> s(N);
    vector<vector<double>> w;
    vector<double> theta;
    double a;
    
    mt19937 gen(time(0));
    uniform_real_distribution<> dis_real(0.0, 1.0);
    uniform_int_distribution<> dis_int(0, N - 1);

    // Aplicamos la regla de aprendizaje hebbiana
    Regla_Hebb(N, a, w, theta, patrones);

    // --- Parámetros de la simulación ---
    const double T = 0.001;      // Temperatura 
    const int Pasos_MC = 50;   // Pasos de Monte Carlo
    const double Ruido = 0.3; // 30% de ruido inicial

    // Abrimos los archivos donde vamos a guardar los datos
    ofstream f_Hopfield("configuraciones_MC.txt");
    ofstream f_solap("solap_prueba_.txt");


    // Inicializamos la red con el primer patrón ruidoso
    //InicializarConPatronRuidoso(s, N, patrones, 0, Ruido, gen);
    InicializarPatronAleatorio(s, N, gen);

    // Guardamos la configuración inicial de la red
     for(int i = 0; i < N; i++) {
            f_Hopfield << s[i] << " ";
        }
        f_Hopfield << "\n";

    // Calculamos y guardamos el solapamiento inicial con cada patrón
    f_solap << 0;
    for (int mu = 0; mu < patrones.size(); mu++) {  
        double m_mu = CalcularSolapamiento(N, a, s, patrones, mu);
        f_solap << 0 << " " << m_mu ;
    }
    f_solap << "\n";


    // Calculamos y mostramos la matriz de correlación entre los patrones
    double correlacion = CalcularMatrizCorrelacion(N, a, patrones, archivos);
    cout << "Matriz de correlacion entre los patrones: " << correlacion << endl;


    // --- BUCLE MONTE CARLO ---
    for (int mcs = 0; mcs < Pasos_MC; mcs++) {
        for (int n = 0; n < N; n++) {
            int i = dis_int(gen); // Neurona al azar
            
            //Calculamos el factor h_i para simplificar los cálculos
            double h_i = 0.0;
            for (int j = 0; j < N; j++) {
                h_i += w[i][j] * s[j];
            }

            double Delta_E = (2.0 * s[i] - 1.0) * (h_i - theta[i]);

            // Metrópolis
            if (Delta_E <= 0 || dis_real(gen) < exp(-Delta_E / T)) {
                s[i] = 1 - s[i];
            }
        }

        // ---  Guardamos los resultados-- 

         for(int i = 0; i < N; i++) {
            f_Hopfield << s[i] << " ";
        }
        f_Hopfield << "\n"; 


        // Guardamos el solapamiento con cada patrón en este paso Monte Carlo
        f_solap << mcs;
        for (int mu = 0; mu < patrones.size(); mu++) {
            double m_mu = CalcularSolapamiento(N, a, s, patrones, mu);
            f_solap << " " << m_mu;
        }
        f_solap << "\n";
    }

    f_Hopfield.close();
    f_solap.close();
    cout << "Simulacion completada." << endl;

    return 0;
}
