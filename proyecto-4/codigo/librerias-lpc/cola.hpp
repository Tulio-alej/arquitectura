#ifndef COLA_HPP
#define COLA_HPP

#include <iostream>

#include "nodo.hpp"

template <typename T>
class Cola
{
private:
    int size;
    Nodo<T> *first;
    Nodo<T> *last;

public:
    // constructores
    Cola() : size(0), first(NULL), last(NULL) {};

    Cola(const Cola<T> &copiar) : size(0), first(NULL), last(NULL)
    {
        if(!this->esVacia()) this->vaciar();

        if (!copiar.esVacia())
        {
            Nodo<T> *actualCopia = copiar.first;
            this->encolar(actualCopia->getInfo());
            actualCopia = actualCopia->getNext();
            while (actualCopia)
            {
                this->encolar(actualCopia->getInfo());
                actualCopia = actualCopia->getNext();
            }
        }
    }

    Cola<T>& operator=(const Cola<T> &otra)
    {
        // 1. Evitar autoasignación
        if (this == &otra) {
            return *this;
        }

        // 2. Limpiar la memoria de la cola actual
        this->vaciar();

        // 3. Copiar los datos de la cola 'otra'
        if (!otra.esVacia())
        {
            Nodo<T> *actualOtra = otra.first;
            while (actualOtra)
            {
                this->encolar(actualOtra->getInfo());
                actualOtra = actualOtra->getNext();
            }
        }

        // 4. Retornar el objeto actual
        return *this;
    }

    // getters
    int getSize() const { return this->size; }

    bool esVacia() const { return !this->first; }

    T getFirst() const
    {
        if (this->esVacia())
        {
            throw std::runtime_error("Error (getFirst): La pila esta vacia");
        }
        else
        {
            return this->first->getInfo();
        }
    }

    T &getFirstRef()
    {
        if (this->esVacia())
        {
            throw std::runtime_error("Error (getFirst): La pila esta vacia");
        }
        else
        {
            return this->first->getInfoRef();
        }
    }

    T getLast() const
    {
        if (this->esVacia())
        {
            throw std::runtime_error("Error (getLast): La cola esta vacia");
        }
        else
        {
            return this->last->getInfo();
        }
    }

    T &getLastRef()
    {
        if (this->esVacia())
        {
            throw std::runtime_error("Error (getLast): La cola esta vacia");
        }
        else
        {
            return this->last->getInfoRef();
        }
    }
    // setters
    void encolar(T dato)
    {
        Nodo<T> *nuevo = new Nodo<T>(dato, NULL);

        if (size == 0)
        {
            this->first = nuevo;
            this->last = nuevo;
        }
        else
        {
            this->last->setNext(nuevo);
            this->last = nuevo;
        }
        this->size++;
    }

    void desencolar()
    {

        if (this->esVacia())
        {
            throw std::runtime_error("Error(desencolar): desencolando cola vacia");
        }

        Nodo<T> *eliminar = this->first;
        if (this->size == 1)
        {
            this->first = NULL;
            this->last = NULL;
        }
        else
        {
            this->first = eliminar->getNext();
        }
        delete eliminar;
        this->size--;
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

    void vaciar()
    {
        while(!this->esVacia()){
            this->desencolar();
        }
    }

    void invertir()
    {
        if(this->esVacia() || this->size == 1)
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

    ~Cola(){
        this->vaciar();
    }
};

#endif
