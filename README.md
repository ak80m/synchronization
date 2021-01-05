# Вопросы Синхронизаци Управления Состояниями Объектов
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
mManager.start() | mManager.stop()

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
