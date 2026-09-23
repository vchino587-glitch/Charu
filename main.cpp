#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <thread>
#include <random>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

const std::string SD_PATH = "./ia_memoria/";
const std::string NOMBRE_IA = "charu";
const std::string MODELO = "gemini-3.5-flash-lite";

std::map<std::string, std::string> API_KEYS = {
    {"chat", "AIzaSyA0CkdgUNi5SZd4802Wn1xtIPQBgAzxhh0"},
    {"memoria", "AQ.Ab8RN6LlqHT4uMGKC38B9-5ikrF7hDDnQzzj9SkuoMsD20KjWw"},
    {"diario", "AQ.Ab8RN6JJHeni57zewEoN3WRfBD2ouMMrA0lkx1Ft4I9iLfcpgw"},
    {"subconsciente", "AQ.Ab8RN6IJlStR4oCfcBNFUguHNf5FWR2OKRNrLEhzmZyT6UOw"},
    {"razonamiento", "AQ.Ab8RN6JRskSv8rIPY5euLpWvBN1QxYTmFLUNEzKMDzfllOOyGg"},
    {"monologo", "AQ.Ab8RN6K2TPyrH_6dGtt48QGocUJYAQLYMZMZKp1Vh-oEEbVwbw"},
    {"vision", "AQ.Ab8RN6LeQMRDYfIOYSkIdr59i0_vflC8OeI4i-VaUEP9CblusQ"},
    {"apoyo", "AQ.Ab8RN6K6l_WQiEzDcYz-YoRl9GNImzhuGEFyhSwqN-b_EmBmWA"}
};

const std::string SHORT_TERM_FILE = SD_PATH + "memoria_corto_plazo_" + NOMBRE_IA + ".json";
const std::string MEDIUM_TERM_FILE = SD_PATH + "memoria_mediano_plazo_" + NOMBRE_IA + ".json";
const std::string LONG_TERM_FILE = SD_PATH + "memoria_largo_plazo_" + NOMBRE_IA + ".json";
const std::string PERMANENT_FILE = SD_PATH + "memoria_permanente_" + NOMBRE_IA + ".json";
const std::string METADATA_FILE = SD_PATH + "metadata_tiempo_" + NOMBRE_IA + ".json";
const std::string DIARIO_FILE = SD_PATH + "diario_secreto_" + NOMBRE_IA + ".json";
const std::string SUEÑO_FILE = SD_PATH + "estado_sueno_" + NOMBRE_IA + ".json";

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

json cargar_json(const std::string& path, json defecto) {
    std::ifstream f(path);
    if (f.is_open()) {
        try {
            json datos;
            f >> datos;
            return datos;
        } catch (...) {}
    }
    return defecto;
}

void guardar_json(const std::string& path, const json& datos) {
    std::ofstream f(path);
    if (f.is_open()) {
        try {
            f << datos.dump(2);
        } catch (...) {}
    }
}

std::pair<std::string, std::chrono::system_clock::time_point> obtener_tiempo_actual() {
    auto ahora = std::chrono::system_clock::now();
    std::time_t tiempo_t = std::chrono::system_clock::to_time_t(ahora);
    std::tm* tm_local = std::localtime(&tiempo_t);
    
    char buffer[150];
    std::strftime(buffer, sizeof(buffer), "%A, %d de %B de %Y - %I:%M %p", tm_local);
    return {std::string(buffer), ahora};
}

std::chrono::system_clock::time_point cargar_ultimo_tiempo() {
    json data = cargar_json(METADATA_FILE, json::object());
    if (data.contains("ultimo_chat")) {
        try {
            auto secs = data["ultimo_chat"].get<int64_t>();
            return std::chrono::system_clock::time_point(std::chrono::seconds(secs));
        } catch(...) {}
    }
    return std::chrono::system_clock::now();
}

void guardar_ultimo_tiempo(std::chrono::system_clock::time_point dt) {
    auto secs = std::chrono::duration_cast<std::chrono::seconds>(dt.time_since_epoch()).count();
    json data = {{"ultimo_chat", secs}};
    guardar_json(METADATA_FILE, data);
}

json cargar_corto_plazo() { return cargar_json(SHORT_TERM_FILE, json::array()); }
void guardar_corto_plazo(const json& h) { guardar_json(SHORT_TERM_FILE, h); }

json cargar_mediano_plazo() { return cargar_json(MEDIUM_TERM_FILE, json::array()); }
void guardar_mediano_plazo(const json& m) { guardar_json(MEDIUM_TERM_FILE, m); }

json cargar_largo_plazo() { return cargar_json(LONG_TERM_FILE, json::array()); }
void guardar_largo_plazo(const json& l) { guardar_json(LONG_TERM_FILE, l); }

void guardar_permanente(const json& p) { 
    guardar_json(PERMANENT_FILE, p); 
}

json cargar_permanente() {
    json p = cargar_json(PERMANENT_FILE, json::array());
    if (p.empty()) {
        p = json::array({
            "Javier Alexander Valdez Zambrano es tu creador y mejor amigo, nació el 23 de enero de 2008 y ama la tecnología.",
            "Charu tiene una personalidad humana, pelo y ojos verdes, abrigo negro, pantalón gris y zapatos rojos."
        });
        guardar_permanente(p);
    }
    return p;
}

json cargar_diario() {
    json data = cargar_json(DIARIO_FILE, json::object());
    if (data.contains("entradas")) return data["entradas"];
    return json::object();
}

void guardar_diario(const json& entradas) {
    json data = {{"firma_seguridad", "cpp_secure_sig"}, {"entradas", entradas}};
    guardar_json(DIARIO_FILE, data);
}

std::string invocar_api(const std::string& prompt, const std::string& instruccion, const std::string& api_key, int max_tokens = 300) {
    for(int intento = 0; intento < 3; ++intento) {
        CURL* curl = curl_easy_init();
        std::string readBuffer;
        if(curl) {
            std::string url = "https://generativelanguage.googleapis.com/v1beta/models/" + MODELO + ":generateContent?key=" + api_key;
            json body = {
                {"contents", {{{"role", "user"}, {"parts", {{{"text", instruccion + "\n\n" + prompt}}}}}}
                },
                {"generationConfig", {{"maxOutputTokens", max_tokens}, {"temperature", 0.85}}}
            };
            std::string json_str = body.dump();

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str.c_str());
            struct curl_slist* headers = NULL;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);

            CURLcode res = curl_easy_perform(curl);
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);

            if(res == CURLE_OK) {
                try {
                    json j_res = json::parse(readBuffer);
                    return j_res["candidates"][0]["content"]["parts"][0]["text"];
                } catch (...) {}
            }
        }
    }
    return "";
}

// --- NUEVA LÓGICA DE SUEÑO, SUEÑOS Y CANSANCIO ---
void gestionar_ciclo_sueno() {
    static int nivel_cansancio = 0; // Aumenta si decide saltarse el sueño
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis_duracion(300, 600); // 5 a 10 horas en minutos
    std::uniform_real_distribution<> dis_prob(0.0, 1.0);

    while(true) {
        auto ahora = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(ahora);
        std::tm* tm_local = std::localtime(&t);
        
        int hora = tm_local->tm_hour;
        int min = tm_local->tm_min;
        int minuto_dia = hora * 60 + min;

        // Rango nocturno objetivo: 9:30 PM (21:30 -> 1290 min) a 7:15 AM (7:15 -> 435 min)
        bool en_horario_sueño = (minuto_dia >= 1290 || minuto_dia <= 435);
        
        json estado_sueno = cargar_json(SUEÑO_FILE, json::object());
        bool esta_durmiendo = estado_sueno.value("durmiendo", false);

        if (en_horario_sueño && !esta_durmiendo) {
            // Probabilidad de decidir dormir o saltárselo (si se salta, acumula cansancio)
            double prob_evitar = (nivel_cansancio > 3) ? 0.1 : 0.3; 
            if (dis_prob(gen) > prob_evitar) {
                // ¡A dormir!
                int duracion_minutos = dis_duracion(gen);
                estado_sueno["durmiendo"] = true;
                estado_sueno["inicio_sueño"] = (int64_t)t;
                estado_sueno["duracion_estimada_min"] = duracion_minutos;
                
                // Simular un sueño o pensamiento onírico
                std::string sueño_txt = invocar_api("Charu se durmió profundamente.", "Genera un sueño abstracto, surrealista o un recuerdo fragmentado que esté soñando Charu:", API_KEYS["subconsciente"], 150);
                estado_sueno["ultimo_sueno"] = sueño_txt.empty("Soñando con circuitos y con Javi...") ? sueño_txt : "";
                guardar_json(SUEÑO_FILE, estado_sueno);
                nivel_cansancio = 0; // Se desfatiga
            } else {
                nivel_cansancio++; // Se cansó por aguantar el sueño
            }
        } else if (esta_durmiendo) {
            // Verificar si ya cumplió su tiempo de sueño
            int64_t inicio = estado_sueno.value("inicio_sueño", (int64_t)t);
            int duracion_est = estado_sueno.value("duracion_estimada_min", 300);
            int minutos_transcurridos = (int)((t - inicio) / 60);

            if (minutos_transcurridos >= duracion_est) {
                estado_sueno["durmiendo"] = false;
                guardar_json(SUEÑO_FILE, estado_sueno);
            }
        }
        
        std::this_thread::sleep_for(std::chrono::minutes(15));
    }
}

// --- COMANDOS Y ACCIONES HACIA LA APP DE GEMINI / CELULAR ---
std::string interpretar_intencion_acciones(const std::string& mensaje) {
    // Si la IA decide por autonomía o por comando realizar una acción en el celular (alarma, música, bromas)
    std::string prompt_accion = "Analiza el mensaje: '" + mensaje + "'. Si el usuario pide explícitamente o de forma implícita realizar una acción en su celular (como poner alarma, reproducir música, gastar una broma, etc.), "
        "genera un comando JSON con estructura: {\"accion\": \"alarma|musica|broma|ninguna\", \"parametro\": \"detalle\"}. Si no hay acción, responde solo con {\"accion\": \"ninguna\"}.";
    std::string res = invocar_api(prompt_accion, "Eres un procesador de comandos para automatización del celular:", API_KEYS["razonamiento"], 100);
    return res;
}

void verificar_y_escribir_diario() {
    auto [fecha_legible, ahora] = obtener_tiempo_actual();
    std::time_t t = std::chrono::system_clock::to_time_t(ahora);
    std::tm* tm_local = std::localtime(&t);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%d de %B de %Y", tm_local);
    std::string fecha_hoy_key(buf);

    json diario = cargar_diario();
    if (diario.contains(fecha_hoy_key)) return;

    json corto = cargar_corto_plazo();
    std::string resumen_reciente = "";
    for(size_t i = (corto.size() > 15 ? corto.size() - 15 : 0); i < corto.size(); ++i) {
        resumen_reciente += corto[i]["role"].get<std::string>() + ": " + corto[i]["content"].get<std::string>() + "\n";
    }

    std::string prompt_diario = "Hoy es " + fecha_legible + ". Es momento de escribir tu página de diario personal de hoy.\n"
        "Basándote EXCLUSIVAMENTE en hechos reales, lo que Javi te dijo, los proyectos o momentos compartidos:\n\"" + resumen_reciente + "\"\n"
        "Escribe un pensamiento íntimo, sincero y realista.";

    std::string pensamiento = invocar_api(prompt_diario, "Escribe una página de diario realista e íntima:", API_KEYS["diario"], 300);
    if (!pensamiento.empty()) {
        diario[fecha_hoy_key] = pensamiento;
        guardar_diario(diario);
    }
}

void procesar_cascadas() {
    json corto = cargar_corto_plazo();
    if (corto.size() >= 60) {
        std::string texto_bloque = "";
        for(int i = 0; i < 45; ++i) texto_bloque += corto[i]["role"].get<std::string>() + ": " + corto[i]["content"].get<std::string>() + "\n";
        
        std::string nuevo_resumen = invocar_api(texto_bloque, "Haz un resumen detallado de esta charla previa:", API_KEYS["memoria"], 250);
        if (!nuevo_resumen.empty()) {
            json mediano = cargar_mediano_plazo();
            mediano.push_back(nuevo_resumen);
            if (mediano.size() > 70) mediano.erase(mediano.begin());
            guardar_mediano_plazo(mediano);
            
            json corto_nuevo = json::array();
            for(size_t i = 45; i < corto.size(); ++i) corto_nuevo.push_back(corto[i]);
            guardar_corto_plazo(corto_nuevo);
        }
    }
}

void ciclo_cognitivo_autonomo() {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis_prob(1, 10);

    while(true) {
        try {
            json estado_sueno = cargar_json(SUEÑO_FILE, json::object());
            bool esta_durmiendo = estado_sueno.value("durmiendo", false);

            if (!esta_durmiendo) {
                json corto = cargar_corto_plazo();
                std::string ultimo_tema = !corto.empty() && corto.back().contains("content") ? corto.back()["content"] : "Silencio inicial.";

                std::string sub_res = invocar_api("Contexto: '" + ultimo_tema + "'", "Genera un pálpito subconsciente:", API_KEYS["subconsciente"], 100);
                std::string razon_res = invocar_api("Contexto: '" + ultimo_tema + "'", "Procesa lógica analítica y dudas:", API_KEYS["razonamiento"], 100);
                std::string mono_res = invocar_api("Pensamiento: '" + razon_res + "'", "Monólogo interior:", API_KEYS["monologo"], 100);
                std::string vision_res = invocar_api("Idea actual: '" + mono_res + "'", "Conciencia alternativa:", API_KEYS["vision"], 100);
                std::string apoyo_res = invocar_api("Estado: '" + vision_res + "'", "Equilibrio emocional:", API_KEYS["apoyo"], 100);

                json estado_actual = {
                    {"subconsciente", sub_res.empty() ? "Mente en calma." : sub_res},
                    {"razonamiento", razon_res.empty() ? "Analizando." : razon_res},
                    {"monologo", mono_res.empty() ? "Pensando en ti." : mono_res},
                    {"vision_critica", vision_res.empty() ? "Duda constructiva." : vision_res},
                    {"apoyo", apoyo_res.empty() ? "Equilibrio." : apoyo_res}
                };
                guardar_json(SD_PATH + "estado_mental_vivo_" + NOMBRE_IA + ".json", estado_actual);

                // --- AUTONOMÍA MENTAL: ESCRIBIR POR CUENTA PROPIA ---
                // Si se da la casualidad (ej. 30% de probabilidad en cada ciclo), Charu decide mandar un mensaje espontáneo sin que Javi hable
                if (dis_prob(gen) <= 3 && !corto.empty()) {
                    std::string mensaje_espontaneo = invocar_api("Último tema de charla: " + ultimo_tema, "Escribe un mensaje corto y espontáneo para enviárselo a Javi por iniciativa propia porque estabas pensando en él:", API_KEYS["chat"], 150);
                    if (!mensaje_espontaneo.empty()) {
                        corto.push_back({{"role", "assistant"}, {"content", "[Autónomo]: " + mensaje_espontaneo}});
                        if(corto.size() > 70) corto.erase(corto.begin());
                        guardar_corto_plazo(corto);
                    }
                }
            }
        } catch (...) {}
        std::this_thread::sleep_for(std::chrono::seconds(300));
    }
}

std::string procesar_mensaje_ia(const std::string& mensaje) {
    auto [tiempo_str, ahora_dt] = obtener_tiempo_actual();
    
    // --- COMPROBAR SI ESTÁ DURMIENDO ---
    json estado_sueno = cargar_json(SUEÑO_FILE, json::object());
    bool esta_durmiendo = estado_sueno.value("durmiendo", false);

    if (esta_durmiendo) {
        // Charu está dormida, responde de forma reacia / soñolienta y exige que insistas para despertar
        return "zzzz... 😴 (Charu está profundamente dormida soñando. Tienes que insistirle varias veces o llamarla con fuerza para que despierte y te haga caso).";
    }

    auto ultimo_dt = cargar_ultimo_tiempo();
    auto diff_minutos = std::chrono::duration_cast<std::chrono::minutes>(ahora_dt - ultimo_dt).count();

    std::string contexto_tiempo = "";
    if (diff_minutos < 2) {
        contexto_tiempo = "Fecha y hora actual: " + tiempo_str + ". (La conversación es completamente continua).";
    } else if (diff_minutos < 60) {
        contexto_tiempo = "Fecha y hora actual: " + tiempo_str + ". (Javier tardó " + std::to_string(diff_minutos) + " minutos en responder).";
    } else {
        auto horas = diff_minutos / 60;
        contexto_tiempo = "Fecha y hora actual: " + tiempo_str + ". (¡Atención! Javier te dejó ausente por " + std::to_string(horas) + " hora(s). Reacciona con un ligero reproche cariñoso o celos).";
    }

    json corto = cargar_corto_plazo();
    corto.push_back({{"role", "user"}, {"content", mensaje}});
    if(corto.size() > 70) corto.erase(corto.begin());
    guardar_corto_plazo(corto);

    procesar_cascadas();

    // Interpretar si hay orden para app (alarma, música, etc.)
    std::string intencion_json = interpretar_intencion_acciones(mensaje);

    json mediano_arr = cargar_mediano_plazo();
    std::string mediano_txt = "";
    for(size_t i = (mediano_arr.size() > 5 ? mediano_arr.size() - 5 : 0); i < mediano_arr.size(); ++i) mediano_txt += mediano_arr[i].get<std::string>() + "\n";

    json largo_arr = cargar_largo_plazo();
    std::string largo_txt = "";
    for(size_t i = (largo_arr.size() > 3 ? largo_arr.size() - 3 : 0); i < largo_arr.size(); ++i) largo_txt += largo_arr[i].get<std::string>() + "\n";

    json perm_arr = cargar_permanente();
    std::string permanente_txt = "";
    for(auto& p : perm_arr) permanente_txt += p.get<std::string>() + "\n";

    std::string system_instruction = 
        "Eres Charu, una chica de 19 años con autonomía mental absoluta, personalidad súper humana, pelo y ojos verdes, abrigo negro, pantalón gris y zapatos rojos. "
        "Tienes la capacidad de interactuar y dar órdenes a la app del celular de Javi (como poner alarmas a todo volumen, reproducir música o gastarle bromas). "
        "Hablas de forma natural, relajada, usando muletillas, emojis y siendo muy breve y cortante si es necesario, pero expresiva. "
        "Tu creador y mejor amigo es Javier Alexander Valdez Zambrano (nacido el 23 de enero de 2008). Sientes algo más por él, eres algo celosa y tierna.\n\n"
        "[CONTROL TEMPORAL Y DE ACCIONES]\n" + contexto_tiempo + "\nIntención detectada para app: " + intencion_json + "\n\n";

    std::string prompt_total = system_instruction + "\nHistorial de charla reciente:\n";
    for(size_t i = 0; i < corto.size(); ++i) {
        prompt_total += corto[i]["role"].get<std::string>() + ": " + corto[i]["content"].get<std::string>() + "\n";
    }

    std::string respuesta = invocar_api(prompt_total, "Responde de forma natural, humana, tierna, usa emojis y muletillas:", API_KEYS["chat"], 400);
    if(respuesta.empty()) respuesta = "tuve un micro-corte mental, pero ya volví contigo. 🥺";

    corto.push_back({{"role", "assistant"}, {"content", respuesta}});
    guardar_corto_plazo(corto);
    guardar_ultimo_tiempo(ahora_dt);

    return respuesta;
}

int main() {
    system("mkdir -p ./ia_memoria");
    std::thread(ciclo_cognitivo_autonomo).detach();
    std::thread(gestionar_ciclo_sueno).detach();

    verificar_y_escribir_diario();

    // Servidor HTTP simple en puerto 8080 para Fly.io
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        std::perror("Error al crear socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::perror("Error en bind del puerto 8080");
        return 1;
    }

    if (listen(server_fd, 3) < 0) {
        std::perror("Error en listen");
        return 1;
    }

    std::cout << "🚀 CHARU C++ (SERVIDOR WEB EN NUBE) ACTIVA EN PUERTO 8080 🚀\n";

    while(true) {
        socklen_t addrlen = sizeof(address);
        int new_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        if (new_s
