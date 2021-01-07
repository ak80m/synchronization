# O Синхронизации

## Старт / Стоп

### Управдение ресурсом через менеджер ресурсов.
Зачастую в программном коде реальных проектов можно встретить подобный менеджер ресурсов.

```
class Manager {
public:
    void start() {
        // start a thread
    }
    void stop() {
        // stop a thread
    }
};
```
### Защита от утечек.
В групповых проектах существует тенденция прокетировать компоненты отдельно друг от друга, 
при этом частенько до того, как будут определены все сценарии использования, так что иногда добавляется код 
такого вида.

```
class Manager {
public:
    Manager() : mStarted(false) {}
    void start() {
        if (mStarted) {
            return;
        }
        // start a thread
        mStarted = true;
    }
    void stop() {
        if (!mStarted) {
            return;
        }
        // stop a thread
        mStarted = false;
    }
private:
    bool mStarted;
};
```

### Многопоточная синхронизация.
По мере того, так разработчик углубляется в особенности многопоточного программирования, рано или поздно появляется "синхронизация".

```
class Manager {
public:
    Manager() : mStarted(false) {}
    void start() {
        std::lock_guard<std::mutex> lk(mMutex);
        if (mStarted) {
            return;
        }
        // start a thread
        mStarted = true;
    }
    void stop() {
        std::lock_guard<std::mutex> lk(mMutex);
        if (!mStarted) {
            return;
        }
        // stop a thread
        mStarted = false;
    }
private:
    std::mutex mMutex;
    bool mStarted;
};
```

Этот правильный с точки зрения `Manager` код застявляет задуматься, а что же на самом деле происходит в программе.
В случае отсутствия внешней синхронизации результат параллельного вызова `start()` и `stop()` непредсказуем.

Thread 1 | Thread 2
---------| --------
`mManager.start()` | `mManager.stop()`

После выполнения такого кода, не смотря на то, что гарантируется целостность внутреннего состояния `Manager`, не известно, создался ли новый поток, или нет.
И как бы мы не старались, внутренними средствами `Manager` решить эту проблему невозможно, 
потому что всегда существует теоретическая вероятность того, что `start()` будет вызван сразу после `stop()`.

### Внешняя синхронизация.

Скорее всего, в программе все-таки существует внешняя синхронизация, и методы `start()` и `stop()` вызываются либо в одном потокe - обработчике событий, 
либо же существует другой мьютекс.

### Нужна ли внутренняя синхронизация?

Скорее всего нужна, но не для нормального выполнения программы, потому что как описано выше, логика нормального вызова `start()/stop()` обычно определяется в отдельном компонте.
Внуртенняя синхронизация может быть полезна для аварийного завершения программы, например в обработчике сигналов.
В таких случаях зачастую невозможно последовательно "раскрутить" полное состояние системы, и вместо этого каждый компонент останавливается и уничтожается отдельно.
Однако `start()/stop()` в этом случае не будет достаточно, так как стоит цель предотвратить любой вызов `start()` после того, как `Manager` будет подготовлен к выходу из программы.

### Terminate & Destroy

Окончательная остановка на то и окончательная, что после нее объект не пригоден к использованию.
Для этого нужен метод `terminate()`.

```
class Manager {
public:
    Manager() : mStarted(false), mTerminated(false) {}
    void start() {
        std::lock_guard<std::mutex> lk(mMutex);
        if (mTerminated) {
            return;
        }
        if (mStarted) {
            return;
        }
        // start a thread
        mStarted = true;
    }
    void stop() {
        std::lock_guard<std::mutex> lk(mMutex);
        if (!mStarted) {
            return;
        }
        // stop a thread
        mStarted = false;
    }
    void terminate() {
        std::lock_guard<std::mutex> lk(mMutex);
        stop();
        mTerminated = true;
    }
private:
    std::mutex mMutex;
    bool mStarted;
    bool mTerminated;
};
```

Метод `terminate()` хоть и не уничтожает объект, но делает его непригодным к дальнейшему использованию.
Так что единственное, что остается после этого, это уничтожить объект и если надо, создать новый.

```
    std::shared_ptr<Manager> m;
    // A sequence of m->start() & m->stop();
    m->terminate();
    m->start(); // does nothing
    m.reset();
```

### Вопрос для самоконтроля.
Имеет ли смысл явное использование мьютекса в конструкторе и деструкторе объектов, которые могут использоваться в `std::shared_ptr`?

```
class Example1 {
public:
    Example1() {
        std::lock_guard<std::mutex> lk(mMutex);
        mStarted = false;
    }
    ~Example1() {
        std::lock_guard<std::mutex> lk(mMutex);
        if (mStarted) {
            // do something
        }
    }
private:
    std::mutex mMutex;
    bool mStarted;
};
 

    std::shared_ptr<Example1> e1;
    std::shared_ptr<Example1> e2 = e1;
    // e1 and e2 and used and released in separate threads
```

## Умные указатели и многопоточность.

### Синхронизация умных указателей.
Не смотря на то, что `std::shared_ptr` использует в своей реализации синхронизацию, сам по себе он не является средством этой синхронизации.

Полезным будет посмотреть следующее видео, в котором, кстати, упоминаются и другие знакомые многим баги: https://youtu.be/lkgszkPnV8g?t=1254
Цитата: _Is shared_ptr thread safe? If you have to ask then NO._

```
class ClassA {
public:
    std::shared_ptr<SharedObject> mSharedObject;
};
```

Следующий код очевидно небезопасный.
Thread 1 | Thread 2| Thread 3
---------| --------|---------
`mSharedObject = `<br>`std::shared_ptr<SharedObject>(new SharedObject());` | `mSharedObject.reset();` | `if (mSharedObject)`<br>`  mSharedObject->method();`

Проблема в том, что и следующий код точно так же небезопасен.
Thread 1 | Thread 2| Thread 3
---------| --------|---------
`mSharedObject = `<br>`std::shared_ptr<SharedObject>(new SharedObject());` | `mSharedObject.reset();` | `std::shared_ptr<SharedObject> sharedObject = `<br>`mSharedObject;`<br>`if (sharedObject) { sharedObject->method(); }`

Объясняется это тем, что реализация всех этих трех операций может быть сделана следующим образом.

Thread 1 | Thread 2| Thread 3
---------| --------|---------
`mSharedObject = `<br>`std::shared_ptr<SharedObject>(new SharedObject());` | `mSharedObject.reset();` | `if (mSharedObject)`<br>` mSharedObject->method();`

* Присваивание.
```
shared_ptr<T>& shared_ptr<T>::operator =(const shared_ptr<T>& other) {
        T *oldPtr = mPtr;
        mPtr = other.mPtr;
        // ...
        return *this;
    }
```

* Освобождение.
```
void shared_ptr<T>::release() {
        T *oldPtr = mPtr;
        mPtr = null_ptr;
        // ...
    }
```

* Копирование.
```
shared_ptr<T>::shared_ptr(const shared_ptr<T>& other) :
        mPtr(other.mPtr) {
        if (mPtr != NULL) {
            // ...
        }
    }
```

Таким образом в предыдущем примере параллельно могут выполняться следующие инструкции.

Thread 1 | Thread 2| Thread 3
---------| --------|---------
`mPtr = other.mPtr;` | `mPtr = null_ptr;` | `if (mPtr != NULL)`

Так что же тогда синхронизировано в `shared_ptr`?

Eдинственное, что он гарантирует, это *существование* объекта, на который он указывает.
А вот то, что он не гарантирует:
* Целостность внутреннего состояния объекта, на который указывают копии `shared_ptr`.
* Целостность одной и той же копии `shared_ptr`, например `mSharedObject`, при одновременном доступе на чтение и изменение.
Во всем том, что не касается освобождение объекта `shared_ptr` - обычный указатель.

Безопасный код может выглядет следующим образом.
```
void init() {
    std::lock_guard<std::mutex> lk(mMutex);
    mSharedObject = std::shared_ptr<SharedObject>(new SharedObject());
}
void deinit() {
    std::lock_guard<std::mutex> lk(mMutex);
    mSharedObject.reset();
}
void method1() {
    std::lock_guard<std::mutex> lk(mMutex);
    if (mSharedObject) { mSharedObject->method(); }
}
void method2() {
    std::shared_ptr<SharedObject> sharedObject;
    { // scope for lock
        std::lock_guard<std::mutex> lk(mMutex);
        sharedObject = mSharedObject;
    }
    if (sharedObject) { sharedObject->method(); }
}
```

### Оптимальная работа с умными указателями.
Нужно избегать создания лишний копий `shared_ptr`.


Пример тривиального, но неоптимального кода.
```
bool val_comparator(std::shared_ptr< SharedObject> i, std::shared_ptr< SharedObject> j) {
    return (i < j); 
}
std::vector<std::shared_ptr< SharedObject>> vector;
std::sort(vector.begin(), vector.end(), val_comparator);
```
Проблема здесь в том, что каждый вызов `val_comparator()` создает две копии `shared_ptr`, что является затратной операцией, так как включает в себя синхронизацию кэша памяти.

Тот же самый код можно значительно ускорить, используя ссылки.
```
bool ref_comparator(std::shared_ptr< SharedObject>& i, std::shared_ptr< SharedObject>& j) {
    return (i < j);
}
std::vector<std::shared_ptr< SharedObject>> vector;
std::sort(vector.begin(), vector.end(), ref_comparator);
```
Копия  `shared_ptr` нужно только тогда, ее жизненный цикл выходит за границы текущей области видимости.
В примере с сортировкой жизненный цикл `i` и `j` не выходит за рамки `val_comparator()`, так что и копии не нужны.





