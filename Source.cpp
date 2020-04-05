#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <ctime>
#include <chrono>
#include <vector>
#include "windows.h"
#include <cmath>
#include <thread>
#include <unordered_map>
#include <map>
#include <algorithm>

enum Status
{
	OK,
	INFECTED,
	DIED
};

class Field
{
public:
	Field(int x_size, int y_size) : m_x_size(x_size), m_y_size(y_size)
	{
	}
	int m_x_size;
	int m_y_size;
};

class Position
{
public:
	Position(double x, double y) : m_x(x), m_y(y)
	{
	}
	Position(Field field) : m_x(rand() % (field.m_x_size + 1)), m_y(rand() % (field.m_y_size + 1))
	{
	}
	double get_dist(Position other)
	{
		return std::sqrt((m_x - other.m_x) * (m_x - other.m_x) + (m_y - other.m_y) * (m_y - other.m_y));
	}
	void randomize_in_field(Field field)
	{
		m_x = rand() % (field.m_x_size + 1);
		m_y = rand() % (field.m_y_size + 1);
	}
	double m_x, m_y;
};

class Human
{
public:
	Human(double x, double y, Status status, double radius) : m_status(status), m_position(x, y), m_shape(radius), m_color(sf::Color::Green), m_target_position(x, y)
	{ 
	}
	void draw(sf::RenderWindow& window)
	{
		m_shape.setFillColor(m_color);
		m_shape.setPosition(m_position.m_x, m_position.m_y);
		window.draw(m_shape);
	}
	void randomize_target_position(Field field)
	{
		m_target_position.randomize_in_field(field);
	}

	void randomize_position(Field field)
	{
		m_position.randomize_in_field(field);
	}

	void step_to_target(double len)
	{
		double diffx = m_target_position.m_x - m_position.m_x, diffy = m_target_position.m_y - m_position.m_y;
		double to_target = sqrt(diffx * diffx + diffy * diffy);
		double coef = (len / to_target) < 1 ? (len / to_target) : 1;
		m_position.m_x += coef * diffx;
		m_position.m_y += coef * diffy;
	}

	void move(double len, Field field, double close_enough)
	{
		if (m_position.get_dist(m_target_position) < close_enough)
			randomize_target_position(field);
		step_to_target(len);

		auto cur_time = std::chrono::system_clock::now();
		int elapsed_seconds= std::chrono::duration_cast<std::chrono::seconds>(cur_time - infected_time).count();

		if (m_status == INFECTED&& elapsed_seconds >= m_seconds_to_die)
		{
				m_color = sf::Color::White;
				m_status = DIED;
		}
	}
	void to_infected(int seconds_to_die)
	{
		m_color = sf::Color::Red;
		m_status = INFECTED;
		infected_time = std::chrono::system_clock::now();
		m_seconds_to_die = seconds_to_die;
	}

public:
	Position m_target_position;
	Status m_status;
	Position m_position;
	sf::CircleShape m_shape;
	sf::Color m_color;
	std::chrono::time_point<std::chrono::system_clock> infected_time;
	int m_seconds_to_die;
};

class Statistic
{
public:
	Statistic(int num_of_moves) :m_num_of_moves(num_of_moves)
	{
	}
	void inc_moves()
	{
		m_num_of_moves++;
	}
	void update(std::vector<Human>& humans)
	{
		int ok=0, infected=0, died=0;
		for (auto& i : humans)
		{
			switch (i.m_status)
			{
			case OK:
				++ok;
				break;
			case INFECTED:
				++infected;
				break;
			case DIED:
				++died;
				break;
			}
		}
		m_stat[m_num_of_moves] = std::make_tuple(ok, infected, died);
	}
	void print_stat()
	{
		for (auto& i : m_stat)
		{
			std::cout << i.first << ";"<< std::get<1>(i.second) << std::endl;
		}
	}
	void draw_gr(sf::RenderWindow& window, Field field, int size)
	{
		field.m_x_size -= 10;
		field.m_y_size -= 10;
		sf::CircleShape shape(size);
		int max_x = (*(m_stat.rbegin())).first;
		int max_y = 0;
		for (auto& i : m_stat)
			max_y = std::max(std::max(std::max(max_y, std::get<0>(i.second)), std::get<1>(i.second)), std::get<2>(i.second));
		double coef_x = field.m_x_size / (double)max_x;
		double coef_y = field.m_y_size / (double)max_y;

		for (auto& i : m_stat)
		{
			shape.setFillColor(sf::Color::Green);
			shape.setPosition(coef_x * i.first, field.m_y_size - coef_y * std::get<0>(i.second));
			window.draw(shape);
			shape.setFillColor(sf::Color::Red);
			shape.setPosition(coef_x * i.first, field.m_y_size - coef_y * std::get<1>(i.second));
			window.draw(shape);
			shape.setFillColor(sf::Color::White);
			shape.setPosition(coef_x * i.first, field.m_y_size - coef_y * std::get<2>(i.second));
			window.draw(shape);
		}
	}
	int m_num_of_moves;
	std::map<int, std::tuple<int, int, int>> m_stat;
};

class Population
{
public:
	Population(Field field, int quantity, double radius) :m_humans(quantity, Human(0, 0, OK, radius)), m_field(field), m_statistic(0)
	{
		for (auto& i : m_humans)
		{
			i.randomize_position(m_field);
			i.randomize_target_position(m_field);
		}
	}

	void infect_random_human(int time_to_die)
	{
		m_humans[rand() % m_humans.size()].to_infected(time_to_die);
	}

	void draw(sf::RenderWindow &window)
	{
		for (auto& i : m_humans)
			i.draw(window);
	}

	void move_all(double len_of_move, double close_enough)
	{
		for (auto& i : m_humans)
			i.move(len_of_move, m_field, close_enough);
		m_statistic.inc_moves();
		m_statistic.update(m_humans);
	}

	void update_infected(double spread_radius, int probability_numerator, int probability_denominator, int seconds_to_die_from, int seconds_to_die_to)
	{
		std::vector<Human> oldhumans = m_humans;
		for (auto& i : oldhumans)
			for (auto& j : m_humans)
			{
				double dist = i.m_position.get_dist(j.m_position);
				if (j.m_status == OK && dist <= spread_radius && i.m_status == INFECTED && (rand() % probability_denominator < probability_numerator))
					j.to_infected(seconds_to_die_from + rand() % (seconds_to_die_to - seconds_to_die_from + 1));
			}
	}

	void update(double len_of_move, double close_enough, double spread_radius, int probability_numerator, int probability_denominator, int seconds_to_die_from, int seconds_to_die_to)
	{
		move_all(len_of_move, close_enough);
		update_infected(spread_radius, probability_numerator, probability_denominator, seconds_to_die_from, seconds_to_die_to);
	}
	Statistic m_statistic;
	std::vector<Human> m_humans;
	Field m_field;
};

int main()
{
	srand(time(nullptr));

	Field field(1500, 900);

	sf::ContextSettings settings;
	settings.antialiasingLevel = 8;
	sf::RenderWindow window(sf::VideoMode(field.m_x_size, field.m_y_size), "COVID-19", sf::Style::Default, settings);

	Population population(field, 1000, 2);
	population.infect_random_human(40);

	int probu = 1, probd = 50, spred_radius = 10, speed = 1;
	while (window.isOpen())
	{
		sf::Event event;
		std::string buf;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
		}
		window.clear();
		population.draw(window);
		window.display();
		population.update(speed, 0.1, spred_radius, probu, probd, 20, 30);
		//std::this_thread::sleep_for(std::chrono::nanoseconds(10));
	}

	population.m_statistic.print_stat();

	sf::RenderWindow window2(sf::VideoMode(field.m_x_size, field.m_y_size), "Graphic", sf::Style::Default, settings);
	while (window2.isOpen())
	{
		sf::Event event;
		while (window2.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window2.close();
		}
		window2.clear();
		population.m_statistic.draw_gr(window2, field, 2);
		window2.display();
	}


	return 0;
}