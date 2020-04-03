#include <SFML/Graphics.hpp>
#include <ctime>
#include <chrono>
#include <vector>
#include "windows.h"
#include <cmath>
#include <thread>

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
	}
	void to_infected()
	{
		m_color = sf::Color::Red;
		m_status = INFECTED;
	}

public:
	Position m_target_position;
	Status m_status;
	Position m_position;
	sf::CircleShape m_shape;
	sf::Color m_color;
};

void move_all(std::vector<Human>& humans, Field field, double len_of_move, double close_enough)
{
	for (auto& i : humans)
		i.move(len_of_move, field, close_enough);
}

void update_infected(std::vector<Human> & humans, double spread_radius, int probability_numerator, int probability_denominator)
{
	std::vector<Human> oldhumans = humans;
	for (auto& i : oldhumans)
		for (auto& j : humans)
		{
			double dist = i.m_position.get_dist(j.m_position);
			if (dist <= spread_radius && i.m_status == INFECTED && (rand() % probability_denominator < probability_numerator))
				j.to_infected();
		}
}

int main()
{
	//std::chrono::time_point<std::chrono::system_clock> start, end;
	//start = std::chrono::system_clock::now();
	srand(time(nullptr));

	Field field(1500, 900);

	sf::ContextSettings settings;
	settings.antialiasingLevel = 8;
	sf::RenderWindow window(sf::VideoMode(field.m_x_size, field.m_y_size), "COVID-19", sf::Style::Default, settings);

	std::vector<Human> humans(1000, Human(0, 0, OK, 2));
	double x = 0;
	double y = 0;
	for (auto& i : humans)
	{
		i.randomize_position(field);
		i.randomize_target_position(field);
	}
	humans[rand() % humans.size()].to_infected();

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
		}
		window.clear();
		for (auto& i : humans)
		{
			i.draw(window);
		}
		window.display();
		move_all(humans, field, 1, 0.1);
		update_infected(humans, 4, 1, 10);
		//std::this_thread::sleep_for(std::chrono::nanoseconds(10));
	}

	//end = std::chrono::system_clock::now();
	//int elapsed_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	//std::time_t end_time = std::chrono::system_clock::to_time_t(end);

	return 0;
}