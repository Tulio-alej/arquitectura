#ifndef PILA_HPP
#define PILA_HPP

#include <iostream>

#include "nodo.hpp"

template <typename T>
class Pila
{
private:
    int size;
    Nodo<T> *tope;

public:
    // constructor
    Pila() : size(0), tope(NULL) {}

    Pila(const Pila<T> &copiar) : size(0), tope(NULL)
    {
        if(!this->esVacia()) this->vaciar();

        if (!copiar.esVacia())
        {
            Nodo<T> *actualOriginal = copiar.tope;                               // con este me paro en la lista original
            Nodo<T> *actualNuevo = new Nodo<T>(actualOriginal->getInfo(), NULL); // este es el nodo nuevo a insertar
            Nodo<T> *anteriorNuevo = actualNuevo;                                // guarda el nodo nuevo anterior para poder enlanzarlo despues
            this->tope = anteriorNuevo;                                          // enlaza el tope

            actualOriginal = actualOriginal->getNext();

            while (actualOriginal)
            {
                actualNuevo = new Nodo<T>(actualOriginal->getInfo(), NULL); // creamos el nodo nuevo
                anteriorNuevo->setNext(actualNuevo);                        // hacemos que el anterior apunte al nuevo
                anteriorNuevo = actualNuevo;                                // cambiamos el anterior al siguiente
                actualOriginal = actualOriginal->getNext();                 // pasamos al nodo siguiente de la lista original
            }

            this->size = copiar.size;
        }
    }

    Pila<T>& operator=(const Pila<T> &otra)
    {
        // 1. Evitar autoasignación
        if (this == &otra) {
            return *this;
        }

        // 2. Limpiar la memoria actual
        this->vaciar();

        // 3. Copiar datos manteniendo el orden
        if (!otra.esVacia())
        {
            Nodo<T> *actualOriginal = otra.tope;
            
            Nodo<T> *actualNuevo = new Nodo<T>(actualOriginal->getInfo(), NULL); 
            Nodo<T> *anteriorNuevo = actualNuevo;                                
            this->tope = anteriorNuevo;                                          

            actualOriginal = actualOriginal->getNext();

            while (actualOriginal)
            {
                actualNuevo = new Nodo<T>(actualOriginal->getInfo(), NULL); 
                anteriorNuevo->setNext(actualNuevo);                        
                anteriorNuevo = actualNuevo;                                
                actualOriginal = actualOriginal->getNext();                 
            }

            this->size = otra.size;
        }

        return *this;
    }

    // getters
    int getSize() const { return this->size; }

    T getDatoTope() const
    {
        if (this->esVacia())
        {
            throw std::runtime_error("Error: La pila esta vacia");
        }
        else
        {
            return this->tope->getInfo();
        }
    }

    Nodo<T> *getTopeRef() { return this->tope; }

    bool esVacia() const
    {
        return !this->tope;
    }

    // setters
    void apilar(T dato)
    {
        Nodo<T> *nuevoNodo = new Nodo<T>(dato, this->tope);
        this->tope = nuevoNodo;
        this->size++;
    }

    void desapilar()
    {
        if (this->esVacia())
        {
            throw std::runtime_error("no hay nada para desapilar.Desapilar");
        }
        else
        {
            Nodo<T> *eliminar = this->tope;
            this->tope = eliminar->getNext();
            delete eliminar;
            this->size--;
        }
    }

    // otros
    void mostrar() const
    {
        Nodo<T> *actual = this->tope;
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
            this->desapilar();
        }
    }

    ~Pila(){
        this->vaciar();
    }
};

#endif