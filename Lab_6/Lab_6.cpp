#include <iostream>
#include <coroutine>
#include <random>
#include <ctime>
#include <chrono>
#include <locale.h>

const int MIN_NUMBER = 1;
const int MAX_NUMBER = 100;

struct GuessNumberPromise;
using GuessNumberHandle = std::coroutine_handle<GuessNumberPromise>;

class GuessNumberCoro;

struct GuessNumberPromise {
    int value_to_yield = 0;

    auto get_return_object();

    auto initial_suspend() noexcept { return std::suspend_never{}; }
    auto final_suspend() noexcept { return std::suspend_always{}; }

    auto yield_value(int value) noexcept {
        value_to_yield = value;
        return std::suspend_always{};
    }

    void return_void() {}
    void unhandled_exception() { std::terminate(); }
};


class GuessNumberCoro {
private:
    GuessNumberHandle handle;

public:
    using promise_type = GuessNumberPromise;

    GuessNumberCoro(GuessNumberHandle h) : handle(h) {}

    GuessNumberCoro(const GuessNumberCoro&) = delete;
    GuessNumberCoro(GuessNumberCoro&& other) noexcept : handle(other.handle) {
        other.handle = nullptr;
    }

    ~GuessNumberCoro() {
        if (handle) handle.destroy();
    }

    bool next() {
        if (handle && !handle.done()) {
            handle.resume();
            return !handle.done();
        }
        return false;
    }

    int get_result() const {
        if (handle) {
            return handle.promise().value_to_yield;
        }
        return 0;
    }

    bool is_done() const {
        return handle.done();
    }
};

inline auto GuessNumberPromise::get_return_object() {
    return GuessNumberCoro(GuessNumberHandle::from_promise(*this));
}


GuessNumberCoro guess_number_coroutine(const int secret_number, int& client_guess_ref) {
    std::cout << "Сопрограма готова до гри.\n";

    int current_result = 1;

    while (current_result != 0) {

        co_await std::suspend_always{};

        int client_guess = client_guess_ref;

        if (client_guess == secret_number) {
            current_result = 0;
            co_yield 0;
            break;
        }
        else if (client_guess < secret_number) {
            current_result = -1;
            co_yield -1;
        }
        else {
            current_result = 1;
            co_yield 1; 
        }
    }

    co_return;
}


int main() {
    std::setlocale(LC_ALL, "ukr");

    std::cout << "### Гра: Вгадай число (Версія 1) ###\n";

    std::mt19937 generator(static_cast<unsigned int>(std::time(0)));
    std::uniform_int_distribution<int> distribution(MIN_NUMBER, MAX_NUMBER);
    const int secret_number = distribution(generator);

    std::cout << "Секретне число успішно згенеровано між " << MIN_NUMBER << " та " << MAX_NUMBER << ".\n";

    int current_guess = 0;
    /*
    //DEBUG: Виводимо загадане число
    std::cout << "[DEBUG] Загадане число: " << secret_number << "\n";
    */
    GuessNumberCoro game = guess_number_coroutine(secret_number, current_guess);

    int attempt_count = 0;
    int result = 1;

    std::cout << "--- Протокол гри ---\n";

    game.next();

    while (result != 0) {
        attempt_count++;

        std::cout << "Спроба " << attempt_count << ": Введіть число (1-" << MAX_NUMBER << "): ";
        if (!(std::cin >> current_guess)) {
            std::cerr << "Помилка введення. Вихід.\n";
            return 1;
        }

        if (current_guess < MIN_NUMBER || current_guess > MAX_NUMBER) {
            std::cout << "   Помилка: Введіть число в діапазоні " << MIN_NUMBER << "-" << MAX_NUMBER << ".\n";
            attempt_count--;
            continue;
        }

        game.next();

        result = game.get_result();

        std::cout << "   Відповідь: ";
        if (result == -1) {
            std::cout << current_guess << " < загаданого числа (-1).\n";
        }
        else if (result == 1) {
            std::cout << current_guess << " > загаданого числа (+1).\n";
        }
        else if (result == 0) {
            std::cout << current_guess << " = загаданого числа (0). **ВГАДАНО!**\n";
        }

        if (game.is_done()) {
            break;
        }
    }

    std::cout << "--- Кінець гри ---\n";
    std::cout << "Гру завершено за " << attempt_count << " спроб.\n";

    return 0;
}