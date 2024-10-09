#!/bin/bash

# Nombres de los ejecutables
SECUENCIAL="./bruteforceS4"
PARALELO="mpirun -np 4 ./bruteforceVar2"

# Claves para prueba (puedes usar operaciones aquí)
KEYS=(1234567 7654321 12345678 876543210 1234567890 9876543210 12345678901)

# Archivo de resultados
RESULTADOS_FILE="resultadosVar2.txt"

# Limpiar el archivo de resultados si ya existe
echo "Clave,Speedup,Tiempo Secuencial (s),Tiempo Paralelo (s),Eficiencia" > "$RESULTADOS_FILE"

# Función para evaluar una expresión matemática
evaluate_key() {
    # $1 -> clave (expresión)
    echo "$1" | bc
}

# Función para ejecutar el programa y extraer el tiempo de ejecución
extract_time() {
    # $1 -> programa a ejecutar
    # $2 -> clave
    output=$($1 "$2")  # Ejecutar el programa y capturar la salida
    echo "$output" | grep "Tiempo de ejecución" | awk '{print $5}'  # Extraer el tiempo de ejecución
}

# Probar con claves
for key_expr in "${KEYS[@]}"; do
    # Evaluar la clave
    key=$(evaluate_key "$key_expr")
    
    echo "Probando con clave: $key (expresión original: $key_expr)"
    
    # Ejecutar el programa secuencial y obtener el tiempo
    echo "Tiempo secuencial:"
    time_sec=$(extract_time "$SECUENCIAL" "$key")
    echo "Tiempo secuencial: $time_sec s"

    # Ejecutar el programa paralelo y obtener el tiempo
    echo "Tiempo paralelo:"
    time_par=$(extract_time "$PARALELO" "$key")
    echo "Tiempo paralelo: $time_par s"

    # Calcular el speedup
    if (( $(echo "$time_par > 0" | bc -l) )); then
        speedup=$(echo "scale=2; $time_sec / $time_par" | bc -l)
    else
        speedup="Inf"  # Manejar caso donde el tiempo paralelo es cero
    fi

    # Calcular la eficiencia
    NUM_PROCESSES=4
    if (( $(echo "$speedup != 0" | bc -l) )); then
        eficiencia=$(echo "scale=2; $speedup / $NUM_PROCESSES" | bc -l)
    else
        eficiencia="0"
    fi

    echo "Speedup: $speedup"
    echo "Eficiencia: $eficiencia"
    
    # Guardar los resultados en el archivo
    echo "$key,$speedup,$time_sec,$time_par,$eficiencia" >> "$RESULTADOS_FILE"

    echo "----------------------------------------"
done

echo "Resultados guardados en $RESULTADOS_FILE"
