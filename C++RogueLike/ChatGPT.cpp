#include "ChatGPT.h" // class ChatGPT


void ChatGPT::start_chat() { // This is the main function of the ChatGPT class
	initscr(); // Initialize the screen
	cbreak(); // Disable line buffering
	echo(); // Echo user input

	while (true) { // Loop until the user exits
		user_input(); // User input
		if (conversation_history.back()["content"].get<std::string>() == "exit") { // Exit if the user types "exit"
					break;
				}
		/*bot_output_goblin();*/ // Goblin output
		/*bot_output_dragon();*/ // Dragon output
		/*bot_output_dungeonmaster();*/ // Dungeon Master output
		/*bot_output_oldsage();*/ // Old Sage output
		bot_output_healthpotion(); // Health Potion output
	}
	noecho(); // Turn off echoing of keys to the screen
	endwin(); // End curses mode
}

void ChatGPT::user_input() { // This function gets user input and adds it to the conversation history
	std::string input{ "" }; // User input
	printw("You: "); // Print the prompt
	char str[524288]; // 512 KiB
	wgetnstr(stdscr, str, 524288); // Get user input
	input = str; // Convert char array to string
	conversation_history.push_back({ {"role", "user"}, {"content", input} }); // Add user input to conversation history
}

void ChatGPT::bot_output_goblin() { // This function gets a response from the goblin chatbot and adds it to the conversation history
	nlohmann::json goblin_history = conversation_history; // Copy the conversation history
	goblin_history.back()["content"] = goblin_history.back()["content"].get<std::string>() // Replace the user's last message with a goblin message
		+ "Respond as: Dungeon Goblin"; // This is the prompt for the goblin chatbot
	nlohmann::json request = { // Create the request
		{"model", "gpt-3.5-turbo"}, // The goblin chatbot
		{"messages", goblin_history}, // The conversation history
		{"max_tokens", 1000},
		{"temperature", 0.7}
	};

	auto response = openai::chat().create(request);
	std::string bot_response = response["choices"][0]["message"]["content"].get<std::string>();
	printw("Goblin: %s\n", bot_response.c_str());
	refresh();
	conversation_history.push_back({ {"role", "assistant"}, {"content", bot_response} });
}

void ChatGPT::bot_output_dragon() { // This function gets a response from the dragon chatbot and adds it to the conversation history
	nlohmann::json dragon_history = conversation_history; // Copy the conversation history
	dragon_history.back()["content"] = dragon_history.back()["content"].get<std::string>() // Replace the user's last message with a dragon message
		+ "Respond as: Dungeon Dragon"; // This is the prompt for the dragon chatbot
	nlohmann::json request = { // Create the request
			{"model", "gpt-3.5-turbo"}, // The dragon chatbot
			{"messages", dragon_history}, // The conversation history
			{"max_tokens", 1000},
			{"temperature", 0.7}
		};
	auto response = openai::chat().create(request);
	std::string bot_response = response["choices"][0]["message"]["content"].get<std::string>();
	printw("Dragon: %s\n", bot_response.c_str());
	refresh();
	conversation_history.push_back({ {"role", "assistant"}, {"content", bot_response} });
}

void ChatGPT::bot_output_dungeonmaster()
{
	nlohmann::json dungeonmaster_history = conversation_history; // Copy the conversation history
	dungeonmaster_history.back()["content"] = dungeonmaster_history.back()["content"].get<std::string>() // Replace the user's last message with a dungeonmaster message
		+ "Respond as: Dungeon Master"; // This is the prompt for the dungeonmaster chatbot
	nlohmann::json request = { // Create the request
				{"model", "gpt-3.5-turbo"}, // The dungeonmaster chatbot
				{"messages", dungeonmaster_history}, // The conversation history
				{"max_tokens", 100},
				{"temperature", 0.7}
			};
	auto response = openai::chat().create(request);
	std::string bot_response = response["choices"][0]["message"]["content"].get<std::string>();
	printw("Dungeon Master: %s\n", bot_response.c_str());
	refresh();
	conversation_history.push_back({ {"role", "assistant"}, {"content", bot_response} });
}

void ChatGPT::bot_output_oldsage()
{
	nlohmann::json oldsage_history = conversation_history; // Copy the conversation history
	oldsage_history.back()["content"] = oldsage_history.back()["content"].get<std::string>() // Replace the user's last message with a oldsage message
		+ "Respond as: Old Sage of The Dungeon"; // This is the prompt for the oldsage chatbot
	nlohmann::json request = { // Create the request
					{"model", "gpt-3.5-turbo"}, // The oldsage chatbot
					{"messages", oldsage_history}, // The conversation history
					{"max_tokens", 1000},
					{"temperature", 0.7}
				};
	auto response = openai::chat().create(request);
	std::string bot_response = response["choices"][0]["message"]["content"].get<std::string>();
	printw("Old Sage: %s\n", bot_response.c_str());
	refresh();
	conversation_history.push_back({ {"role", "assistant"}, {"content", bot_response} });
}

void ChatGPT::bot_output_healthpotion()
{
	nlohmann::json healthpotion_history = conversation_history; // Copy the conversation history
	healthpotion_history.back()["content"] = healthpotion_history.back()["content"].get<std::string>() // Replace the user's last message with a healthpotion message
		+ "Respond as: Birthday Card"; // This is the prompt for the healthpotion chatbot
	nlohmann::json request = { // Create the request
						{"model", "gpt-3.5-turbo"}, // The healthpotion chatbot
						{"messages", healthpotion_history}, // The conversation history
						{"max_tokens", 1000},
						{"temperature", 0.7}
					};
	auto response = openai::chat().create(request);
	std::string bot_response = response["choices"][0]["message"]["content"].get<std::string>();
	printw("Birthday Card: %s\n", bot_response.c_str());
	refresh();
	conversation_history.push_back({ {"role", "assistant"}, {"content", bot_response} });
}
