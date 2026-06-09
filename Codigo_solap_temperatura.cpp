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
                
                if (c == '1') {
                    patron.push_back(1);
                } else if (c == '0') {
                    patron.push_back(0);
                }
            }
        }
    N_calculado = patron.size(); // Actualizamos el tamaño N
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
    
    int N = 0;
    vector<int> patron;
    CargarPatrones("tigre.txt", patron, N);
    
    if (N == 0) {
        cerr << "Error (N=0)" << endl;
        return 1;
    }

    // Parámetros de la red 
    vector<int> s(N);
    vector<vector<double>> w;
    vector<double> theta;
    double a;
    
    mt19937 gen(time(0));
    uniform_real_distribution<> dis_real(0.0, 1.0);
    uniform_int_distribution<> dis_int(0, N - 1);

    // Aplicamos la regla de Hebb para encontrar los pesos    
    Regla_Hebb(N, a, w, theta, patron);

    // Abrimos el archivo donde guardamos los datos    
    ofstream f_barrido("barrido_temperatura.txt");

    // --- Definimos los parametros para barrer la temperatura (escala logaritmica) ---
    double T_inicio = 0.01;
    double T_final = 1.0;
    double factor = 1.15; // Factor multiplicativo para la escala logarítmica

    // Imprimimos la cabecera de la tabla
    cout << "T\t\tSolapamiento" << endl;
    cout << "-----------------------------" << endl;

    for (double T = T_inicio; T <= T_final; T *= factor) {
        
        // Inicializamos con un patron aleatorio
        InicializarAleatorio(s, N, gen); 

        // Calculamos los campos locales, una vez para cada temperatura   
        vector<double> h(N, 0.0);
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                h[i] += w[i][j] * s[j];
            }
        }

        // --- EVOLUCIÓN TEMPORAL ---
        int Pasos_MC = 10; 
        for (int mcs = 0; mcs < Pasos_MC; mcs++) {
            for (int n = 0; n < N; n++) {
                
                // Elegimos una neurona al azar
                int i = dis_int(gen);
                
                // Calculamos la delta de energia  
                double Delta_E = (2.0 * s[i] - 1.0) * (h[i] - theta[i]);

                // Metrópolis
                if (Delta_E <= 0 || dis_real(gen) < exp(-Delta_E / T)) {
                    
                    // Definimos una forma más eficiente de actualizar los campos locales para no tener que hacer la sumatoria cada vez
                    // Necesitamos esta modificación con respecto a los codigos Hopfield-patron 
                    // porque aquí tenemos que hacer este proceso para un bucle amplio de temperaturas
                    // por lo que la eficiencia se ve muy afectada si no actualizamos los campos locales con incrementos en vez de recalcularlos cada vez
            

                    int diferencia_estado; // Variable para almacenar la diferencia entre el nuevo estado y el antiguo
                    if (s[i] == 0) {
                        diferencia_estado = 1;  // Cambio de 0 a 1
                    } else {
                        diferencia_estado = -1; // Cambio de 1 a 0
                    }

                    // Actualizamos el estado de la neurona
                    s[i] = 1 - s[i];

                    // Actualización de h_i usando incrementos en vez de recalcularlos cada vez
                    for (int j = 0; j < N; j++) {
                        if (i != j) {
                            h[j] += w[j][i] * diferencia_estado;
                        }
                    }
                }
            }
        }

        // ---  Calculamos el solapamiento final después de la evolución temporal ---
        double m_final = CalcularSolapamiento(N, a, s, patron);
        
        f_barrido << T << " " << m_final << endl;
        cout << T << "\t\t" << m_final << endl;
    }

    f_barrido.close();
    cout << "-----------------------------" << endl;
    cout << "Simulacion finalizada. Datos guardados en barrido_temperatura.txt" << endl;

    return 0;
}
