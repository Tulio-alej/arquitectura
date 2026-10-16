#ifndef NODO_HPP
#define NODO_HPP

#include <iostream>

template <typename T>
class Nodo
{
private:
    T info;
    Nodo<T> *next;

public:
    // constructor
    Nodo(T info, Nodo<T> *next) : info(info), next(next) {}
    // setters
    T getInfo() const { return this->info; }
    T& getInfoRef() { return this->info; }
    Nodo<T> *getNext() { return this->next; }
    // getters
    void setInfo(T elemento) { this->info = elemento; }
    void setNext(Nodo<T> *siguiente) { this->next = siguiente; }
    // destructor
    ~Nodo() {}
};

#endif