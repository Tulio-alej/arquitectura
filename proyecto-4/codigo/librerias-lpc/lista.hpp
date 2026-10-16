#ifndef LISTA_HPP
#define LISTA_HPP

#include <iostream>

#include "nodo.hpp"

template <typename T>
class Lista
{
private:
    Nodo<T> *first;
    Nodo<T> *last;
    int length;

    //-------------------funciones auxiliares para el merge---------------------------
    // Separa la lista a la mitad y devuelve el inicio de la segunda mitad
    Nodo<T> *obtenerMitad(Nodo<T> *cabeza)
    {
        if (!cabeza)
            return cabeza;

        Nodo<T> *lento = cabeza;
        Nodo<T> *rapido = cabeza->getNext();

        while (rapido && rapido->getNext())
        {
            lento = lento->getNext();
            rapido = rapido->getNext()->getNext();
        }

        Nodo<T> *mitad = lento->getNext();
        lento->setNext(NULL); // Rompe la conexión entre ambas mitades
        return mitad;
    }

    // Mezcla dos listas ordenadas recursivamente o iterativamente
    Nodo<T> *mezclar(Nodo<T> *izq, Nodo<T> *der)
    {
        if (!izq)
            return der;
        if (!der)
            return izq;

        Nodo<T> *resultado = NULL;

        if (izq->getInfo() <= der->getInfo())
        {
            resultado = izq;
            resultado->setNext(mezclar(izq->getNext(), der));
        }
        else
        {
            resultado = der;
            resultado->setNext(mezclar(izq, der->getNext()));
        }

        return resultado;
    }

    // Función recursiva principal del MergeSort
    Nodo<T> *mergeSortRec(Nodo<T> *cabeza)
    {
        // Caso base: lista vacía o con un solo elemento
        if (!cabeza || !cabeza->getNext())
        {
            return cabeza;
        }

        Nodo<T> *mitad = obtenerMitad(cabeza);

        Nodo<T> *izq = mergeSortRec(cabeza);
        Nodo<T> *der = mergeSortRec(mitad);

        return mezclar(izq, der);
    }

    //--------------------------------------------------------------

public:
    // constructores
    Lista() : first(NULL), last(NULL), length(0) {}

    Lista(const Lista<T> &copiar) : first(NULL), last(NULL), length(0)
    {
        
        if (!copiar.esVacia())
        {
            Nodo<T> *actualCopia = copiar.first; // creamos el nodo actual
            this->insertar(actualCopia->getInfo(), 0);
            actualCopia = actualCopia->getNext();
            while (actualCopia)
            {
                this->insertar(actualCopia->getInfo(), -1);
                actualCopia = actualCopia->getNext();
            }
        }
    }

    Lista<T>& operator=(const Lista<T> &otra)
    {
        // 1. Evitar autoasignación
        if (this == &otra) {
            return *this;
        }

        // 2. Limpiar la memoria actual
        this->vaciar();

        // 3. Copiar datos
        if (!otra.esVacia())
        {
            Nodo<T> *actualOtra = otra.first;
            
            // Insertar el primero en la pos 0
            this->insertar(actualOtra->getInfo(), 0);
            actualOtra = actualOtra->getNext();
            
            // Insertar el resto al final (pos -1) para que sea rápido
            while (actualOtra)
            {
                this->insertar(actualOtra->getInfo(), -1);
                actualOtra = actualOtra->getNext();
            }
        }

        return *this;
    }

    // getters
    int getLength() const { return this->length; }

    Nodo<T> *getFirst() const { return this->first; }

    Nodo<T> *getLast() const { return this->last; }

    bool esVacia() const
    {
        return !first;
    }

    T getPos(int n) const
    {
        if (this->esVacia())
        {
            throw std::runtime_error("ERORR: la lista es vacia (getPos)");
        }

        if (n > this->length - 1 || n < 0)
        {
            throw std::runtime_error("ERORR: indice invalido (getPos)");
        }

        if (n == -1 || n == this->length - 1)
        {
            return this->last->getInfo();
        }

        if (n == 0)
        {

            return this->first->getInfo();
        }

        Nodo<T> *actual = this->getFirst()->getNext();
        int aux = 1;
        while (aux < n)
        {
            actual = actual->getNext();
            aux++;
        }

        return actual->getInfo();
    }

    T &getPosRef(int n)
    {
        if (this->esVacia())
        {
            throw std::runtime_error("ERROR: la lista es vacia (getPos)");
        }

        if (n > this->length - 1 || n < 0)
        {
            throw std::runtime_error("ERROR: indice invalido (getPos)");
        }

        if (n == -1 || n == this->length - 1)
        {
            return this->getLast()->getInfoRef();
        }

        if (n == 0)
        {

            return this->getFirst()->getInfoRef();
        }

        Nodo<T> *actual = this->getFirst()->getNext();
        int aux = 1;
        while (aux < n)
        {
            actual = actual->getNext();
            aux++;
        }

        return actual->getInfoRef();
    }
    
    T getPosCircular(int pos, int posInicial) const
    {
        if (this->esVacia())
        {
            throw std::runtime_error("ERROR: la lista está vacía (eliminarCircular)");
        }
    
        // Normaliza la posición inicial dentro del rango válido de la lista [0, length - 1]
        int inicioNormalizado = ((posInicial % this->length) + this->length) % this->length;
    
        // Suma la posición relativa deseada al punto de partida
        int posAbsoluta = inicioNormalizado + pos;
    
        // Normaliza el resultado final dentro de [0, length - 1]
        int posReal = ((posAbsoluta % this->length) + this->length) % this->length;
    
        // Reutiliza tu método getPos para realizar el borrado físico
        return this->getPos(posReal);

        
    }

    T &getPosCircularRef(int pos, int posInicial)
    {
        if (this->esVacia())
        {
            throw std::runtime_error("ERROR: la lista está vacía (eliminarCircular)");
        }
    
        // Normaliza la posición inicial dentro del rango válido de la lista [0, length - 1]
        int inicioNormalizado = ((posInicial % this->length) + this->length) % this->length;
    
        // Suma la posición relativa deseada al punto de partida
        int posAbsoluta = inicioNormalizado + pos;
    
        // Normaliza el resultado final dentro de [0, length - 1]
        int posReal = ((posAbsoluta % this->length) + this->length) % this->length;
    
        // Reutiliza tu método getPos para realizar el borrado físico
        return this->getPosRef(posReal);
    }

    int getIndiceCircular(int k, int posInicial) const
    {
        // 1. Verificación de lista vacía
        if (this->esVacia())
        {
            throw std::runtime_error("ERROR: La lista está vacía (eliminarCircular)");
        }

        // 2. Normalización de posInicial
        // El doble módulo garantiza un índice positivo en el rango [0, length - 1]
        int inicioNormalizado = ((posInicial % this->length) + this->length) % this->length;

        // 3. Cálculo del índice absoluto sumando el desplazamiento k
        int posAbsoluta = inicioNormalizado + k;

        // 4. Normalización del índice final dentro del rango circular
        int posReal = ((posAbsoluta % this->length) + this->length) % this->length;
        return posReal;
    }

    int encontrar(T buscar) const
    {
        Nodo<T> *actual = this->first;
        int pos = 0;

        while (actual != NULL)
        {
            if (actual->getInfo() == buscar)
            {
                return pos; // Retorna la posición de la primera coincidencia
            }
            actual = actual->getNext();
            pos++;
        }

        return -1; // Retorna -1 si no se encontró el elemento
    }
    

    // setters
    void insertar(T element, int pos)
    {
        if (pos == -1)
        {
            pos = this->length;
        }

        if (this->esVacia() && pos != 0)
        {
            throw std::runtime_error("ERROR: lista vacia y posicion no valida para insertar (func insertar)");
        }
        else if (pos < 0 || pos > this->length)
        {
            throw std::runtime_error("ERROR: posicion de insercion no valida (func insertar)");
        }
        // ahora si la insercion
        else if (this->esVacia() && pos == 0)
        {
            Nodo<T> *nuevo = new Nodo<T>(element, NULL);
            this->first = nuevo;
            this->last = nuevo;
        }
        else if (pos == 0) // insertar al inicio
        {
            Nodo<T> *nuevo = new Nodo<T>(element, NULL);
            nuevo->setNext(this->first);
            this->first = nuevo;
        }
        else if (pos == this->length)
        {
            Nodo<T> *nuevo = new Nodo<T>(element, NULL);
            this->last->setNext(nuevo);
            this->last = nuevo;
        }
        else
        {
            int i = 0;
            Nodo<T> *nuevo = new Nodo<T>(element, NULL);
            Nodo<T> *actual = this->first;
            while (i < pos - 1)
            {
                actual = actual->getNext();
                i++;
            }
            nuevo->setNext(actual->getNext());
            actual->setNext(nuevo);
        }
        this->length++;
    }

    void eliminar(int pos)
    {
        if (pos == -1)
        {
            pos = this->length - 1;
        }

        if (this->esVacia())
        {
            throw std::runtime_error("ERROR: lista vacia y posicion no valida para eliminar (func eliminar");
        }
        else if (pos < 0 || pos >= this->length)
        {
            throw std::runtime_error("ERROR: posicion de eliminacion no valida (func eliminar)");
        }

        Nodo<T> *actual = this->first;

        if (this->length == 1 && pos == 0)
        {
            delete this->first;
            this->last = NULL;
            this->first = NULL;
        }
        else if (pos == 0) // primer elemento
        {
            Nodo<T> *eliminar = actual;
            actual = actual->getNext();
            this->first = actual;
            delete eliminar;
        }
        else
        {
            Nodo<T> *eliminar = actual->getNext();
            int i = 0;
            while (i < pos - 1)
            {
                eliminar = eliminar->getNext();
                actual = actual->getNext();
                i++;
            }

            actual->setNext(eliminar->getNext());
            if (eliminar->getNext() == NULL)
            {
                this->last = actual;
            }
            delete eliminar;
        }

        this->length--;
    }

    void eliminarCircular(int k, int posInicial)
{
    // 1. Verificación de lista vacía
    if (this->esVacia())
    {
        throw std::runtime_error("ERROR: La lista está vacía (eliminarCircular)");
    }

    // 2. Normalización de posInicial
    // El doble módulo garantiza un índice positivo en el rango [0, length - 1]
    int inicioNormalizado = ((posInicial % this->length) + this->length) % this->length;

    // 3. Cálculo del índice absoluto sumando el desplazamiento k
    int posAbsoluta = inicioNormalizado + k;

    // 4. Normalización del índice final dentro del rango circular
    int posReal = ((posAbsoluta % this->length) + this->length) % this->length;

    // 5. Ejecución del borrado físico reusando el método eliminar(int)
    this->eliminar(posReal);
}
    // otros
    void mostrar() const
    {
        Nodo<T> *actual = this->first;
        while (actual)
        {
            std::cout << actual->getInfo() << "->";
            actual = actual->getNext();
        }
        std::cout << "NULL\n";
    }

    void mostrarEspecial(char c) const
    {
        Nodo<T> *actual = this->first;
        while (actual)
        {
            std::cout << actual->getInfo() << c;
            actual = actual->getNext();
        }
        std::cout << "\n";
    }

    void vaciar()
    {
        Nodo<T> *eliminar = this->first;
        if (this->esVacia())
        {
        }
        else if (this->getLength() == 1)
        {
            delete eliminar;
        }
        else
        {
            Nodo<T> *siguiente = eliminar->getNext();
            while (siguiente)
            {
                delete eliminar;
                eliminar = siguiente;
                siguiente = siguiente->getNext();
            }
            delete eliminar;
        }
        this->first = NULL;
        this->last = NULL;
        this->length = 0;
    }

    void ordenar()
    {
        if (this->length <= 1)
            return;

        // Ejecuta el MergeSort reordenando punteros
        this->first = mergeSortRec(this->first);

        // Actualiza el puntero `last` para mantener consistencia en la lista
        Nodo<T> *aux = this->first;
        while (aux && aux->getNext())
        {
            aux = aux->getNext();
        }
        this->last = aux;
    }

    T moda() const
    {
        if (this->esVacia())
        {
            throw std::runtime_error("ERROR: no existe moda(func moda)");
        }

        Nodo<T> *actual = this->first;
        T moda_elemento = actual->getInfo();
        int max_repeticiones = 0;

        while (actual)
        {
            T elemento_candidato = actual->getInfo();
            int repeticiones_actuales = 0;
            
            // Recorremos la lista completa para contar cuántas veces aparece el candidato
            Nodo<T> *aux = this->first; 
            while (aux != NULL)
            {
                if (aux->getInfo() == elemento_candidato)
                {
                    repeticiones_actuales++;
                }
                aux = aux->getNext();
            }

            // Si este candidato aparece más veces que el máximo registrado, toma el trono
            if (repeticiones_actuales > max_repeticiones)
            {
                max_repeticiones = repeticiones_actuales;
                moda_elemento = elemento_candidato;
            }
            
            actual = actual->getNext();
        }

        return moda_elemento;
    }

    void invertir()
    {
        if(this->esVacia() || this->length == 1)
        {
            
        }
        else
        {
            Nodo<T>* anterior = NULL;
            Nodo<T>* actual = this->first;
            Nodo<T>* siguiente = actual->getNext();

            while(siguiente)
            {
                actual->setNext(anterior); //actual apunta al anterior
                anterior = actual;
                actual = siguiente;
                siguiente = siguiente->getNext();
            }
            actual->setNext(anterior);
            Nodo<T>* aux = this->first;
            this->first = this->last;
            this->last = aux;


        }
    }



    void inicializar(int desde, int hasta)
    {
        for (int i=desde ; i<=hasta ; i++)
        {
            this->insertar(i,-1);
        }
    }

    ~Lista(){
        this->vaciar();
    }
};

#endif