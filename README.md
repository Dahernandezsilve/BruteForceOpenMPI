# BruteForceOpenMPI
🫳 Implementación y diseño de programas para la paralelización de procesos utilizando memoria para descubrir la llave privada con la que se cifró un texto mediante el método de fuerza bruta. 💻 

## 💻 Funcionalidades
- Makefile base para los integrantes.
- Algoritmo de fuerza bruta que intenta descrifrar llaves privadas para encontrar un texto escrito en un archivo txt.
- Algoritmo DES para la desencriptación de textos en una clave de 56 bits.
- Algoritmo AES para la desencriptación de textos en una clave de 128 bits.
- Algoritmo ChaCha20 para la desencriptación de textos en una clave de 256 bits.

## 🏛️ Información
OpenMPI es una librería de código abierto de Message Passing Interface, una especificación utilizada en computación paralela para clústers o para programación distribuida.

## 🎯 Objetivos y Competencias
- Implementar y diseñar un programa que por medio del algoritmo de fuerza bruta, sea capaz de encontrar una llave privada y desencriptar un mensaje en un archivo de texto.
- Utilizar el algoritmo DES sugerido en las especificaciones del proyecto y la implementación de dos alternativas más.
- Realizar pruebas de speedup y eficiencia para encontrar la mejor versión y acercamiento posible.

## 📋 Requisitos
- Código de autoría propia en C/C++ con comentarios explicativos. 🖋️
- Historial del control de versiones del programa iniciando 2 semanas antes de la entrega (privado hasta el día de entrega). 📂
- Uso de OpenMPI. 🚀
- Versiones secuencial y paralela(s) del algoritmo y de los acercamientos propuestos. 🔄
- Las mediciones y elementos requeridos indicados en las instrucciones del proyecto.

## 📚 Contenido
Se realizó una investigación sobre el algoritmo DES y se logró utilizar la librería gcrypt para poder hacer funcionar el programa bruteforce_Inicial.c. Posteriormmente, se solicita realizar una versión secuencial
y paralela utilizando este algoritmo y realizando el proceso por fuerza bruta. Después de realizar esto y unas pruebas, se solicita realizar otras dos aproximaciones distintas con otros algoritmos, para que,
de esta manera, se pueda comparar a raíz de esto la mejor versión y el mejor acercamiento, probando con distintas llaves y longitudes.

### 🛠️ Herramientas y Tecnologías
- OpenMPI
- GCrypt (para encriptar y desencriptar texto)
- Algoritmo DES
- Algoritmo AES
- Algoritmo ChaCha20
- Manejo adecuado y uso de memoria

### ⏳ Instrucciones de Compilación y Ejecución
Para compilar y ejecutar este proyecto, es conveniente utilizar el Makefile que se encuentra entre los archivos del mismo. Utilizando los comandos:

- make
- make run

Serán capaces de poder compilar y ejecutar uno por uno los programas utilizados para las pruebas de speedup y eficiencia, como también los algoritmos utilizados en cada uno de ellos.

### 💡 Recomendaciones
- Se recomienda antes de iniciar con una implementación de esta manera, hacer una investigación acerca de los algoritmos y cómo se puede hacer un acercamiento viable para encontrar alternativas viables en base a la eficiencia del algoritmo.
- Es conveniente procurar prestar atención a las métricas para determinar la mejora o el rendimiento de un algoritmo respecto a otro o a su versión secuencial, para poder determinar cómo poder mejorarlo.
- Revisar librerías y funciones que permitan realizar una mejor comunicación entre procesos y una mejor distribución entre ellos para mejorar la eficiencia del algoritmo implementado.



## 👨‍🏫 Docente
Sebastián Galindo

## 🏫 Universidad del Valle de Guatemala
Computación Paralela y Distribuida

Semestre 2, 2024
