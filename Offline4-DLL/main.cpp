#include "cmath"
#include "iostream"
#include "string"
#include "vector"

using namespace std;

// returns the ASCII value of the character
string char_to_binary(char c) {
  string binary = "";
  for (int i = 7; i >= 0; i--) {
    binary += to_string((c >> i) & 1);
  }
  return binary;
}

string pad_string(string str, int m, char padding_char = '~') {
  if (str.length() % m == 0) {
    return str;
  } else {
    int padding_length = m - (str.length() % m);
    for (int i = 0; i < padding_length; i++) {
      str += padding_char;
    }
    return str;
  }
}

string string_to_binary(string str) {
  string binary = "";
  for (int i = 0; i < str.length(); i++) {
    binary += char_to_binary(str[i]);
  }
  return binary;
}

string binary_to_string(string binary) {
  string str = "";
  for (int i = 0; i < binary.length(); i += 8) {
    int c = 0;
    for (int j = 0; j < 8; j++) {
      c += (binary[i + j] - '0') * (1 << (7 - j)); // just simple weighted sum
    }
    str += (char)c;
  }
  return str;
}

void print_binary(string binary, int m) {
  for (int i = 0; i < binary.length(); i++) {
    cout << binary[i];
    if ((i + 1) % (8 * m) == 0) {
      cout << endl;
    }
  }
}

vector<string> split_binary(string binary, int m) {
  vector<string> binary_split;
  for (int i = 0; i < binary.length(); i += 8 * m) {
    binary_split.push_back(binary.substr(i, 8 * m));
  }
  return binary_split;
}

void print_binary(vector<string> binary_split) {
  for (int i = 0; i < binary_split.size(); i++) {
    cout << binary_split[i] << endl;
  }
}

int number_of_redundancy_bits(int m) {
  long data_bits = 8 * m;
  int r = 0;
  while ((1 << r) < data_bits + r + 1) {
    r++;
  }
  return r;
}

// prints the redundant bits in green for a row only
void print_redundant_binary(string binary) {
  // the redundant bits are at the positions of the powers of 2, make their
  // color green
  int r = 0;
  for (int i = 0; i < binary.length(); i++) {
    if (((1 << r) - 1) == i) {
      cout << "\033[1;36m" << binary[i] << "\033[0m";
      r++;
    } else {
      cout << binary[i];
    }
  }
}

string add_redundancy_bits(string binary, int m) {
  int r = number_of_redundancy_bits(m);

  // just fixing up the positions of the bits
  string redundant_binary = "";
  int j = 0;
  for (int i = 0; i < binary.length() + r; i++) {
    if (i == pow(2, j) - 1) {
      redundant_binary += '0';
      j++;
    } else {
      redundant_binary += binary[i - j];
    }
  }

  // now calculate the redundancy bits
  // for (int i = 0; i < r; i++) {
  //     int redundancy_bit = 0;
  //     for (int j = pow(2,i) - 1; j < redundant_binary.length(); j++) {
  //         if (redundant_binary[j] == '1' && ((j + 1) & (1 << i)) != 0) {
  //             redundancy_bit = (redundancy_bit + 1) % 2;
  //         }
  //     }
  //     redundant_binary[pow(2, i) - 1] = redundancy_bit + '0';
  // }

  for (int i = 0; i < r; i++) {
    int step =
        1 << i; // for r1, we go by 1 step, for r2, we go by 2 steps, etc.

    bool redundancy_bit = false;
    for (j = (1 << i) - 1; j < redundant_binary.length(); j += step * 2) {
      for (int k = 0; k < step; k++) {
        if (redundant_binary[j + k] == '1') {
          redundancy_bit = !redundancy_bit;
        }
      }
    }
    redundant_binary[(1 << i) - 1] = redundancy_bit + '0';
  }

  return redundant_binary;
}

vector<string> generate_redudancy_bit_string(vector<string> data_block, int m) {
  vector<string> redundant_data_block;
  for (int i = 0; i < data_block.size(); i++) {
    redundant_data_block.push_back(add_redundancy_bits(data_block[i], m));
  }
  return redundant_data_block;
}

void print_redundant_bit_string(vector<string> redundant_block) {
  for (int i = 0; i < redundant_block.size(); i++) {
    print_redundant_binary(redundant_block[i]);
    cout << endl;
  }
}

string columnwise_serialize(vector<string> redundant_block) {
  int n = redundant_block[0].length();

  string serialized_string = "";
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < redundant_block.size(); j++) {
      serialized_string += redundant_block[j][i];
    }
  }

  return serialized_string;
}

string verify_checksum(string received_string, string g) {
  int l = g.length();

  // remainder is initialized to the first l bits of the serialized string
  string remainder = received_string.substr(0, l);
  int n = received_string.length();

  //   cout << remainder << endl;

  int k = l; // k --> next bit to be considered in calculation
  while (k < n) {
    string temp = "";
    for (int i = 1; i < l; i++) {
      if (remainder[0] == '1') {
        temp += (remainder[i] == g[i]) ? '0' : '1';
      } else {
        temp += remainder[i];
      }
    }
    temp += received_string[k++];
    remainder = temp;
    // cout << remainder << endl;
  }

  // for the last iteration
  string temp = "";
  for (int i = 1; i < l; i++) {
    if (remainder[0] == '1') {
      temp += (remainder[i] == g[i]) ? '0' : '1';
    } else {
      temp += remainder[i];
    }
  }

  string checksum = temp;

  return checksum;
}

string add_and_print_checksum(string serialized_string, string g) {
  int l = g.length();

  // padding l-1 zeros to the end of the serialized string
  for (int i = 0; i < l - 1; i++) {
    serialized_string += '0';
  }

  // remainder is initialized to the first l bits of the serialized string
  string remainder = serialized_string.substr(0, l);
  int n = serialized_string.length();

  //   cout << remainder << endl;

  int k = l; // k --> next bit to be considered in calculation
  while (k < n) {
    string temp = "";
    for (int i = 1; i < l; i++) {
      if (remainder[0] == '1') {
        temp += (remainder[i] == g[i]) ? '0' : '1';
      } else {
        temp += remainder[i];
      }
    }
    temp += serialized_string[k++];
    remainder = temp;
    // cout << remainder << endl;
  }

  // for the last iteration
  string temp = "";
  for (int i = 1; i < l; i++) {
    if (remainder[0] == '1') {
      temp += (remainder[i] == g[i]) ? '0' : '1';
    } else {
      temp += remainder[i];
    }
  }

  string checksum = temp;

  for (int i = l - 2, j = 1; i >= 0; i--, j++) {
    serialized_string[n - j] = checksum[i];
    // cout << checksum[i] << endl;
  }

  // print serialized string with checksum, the checksum bits are in cyan blue
  cout << "\ndata bits after appending CRC checksum <sent-frame>:" << endl;
  for (int i = 0; i < n; i++) {
    if (i > n - l) {
      cout << "\033[34m" << serialized_string[i] << "\033[0m";
    } else {
      cout << serialized_string[i];
    }
  }

  cout << endl;

  return serialized_string;
}

vector<string> check_and_remove_checksum(string received_string, string g,
                                         int row_length) {
  string checksum = verify_checksum(received_string, g);

  bool no_error = true;
  for (int i = 0; i < checksum.length(); i++) {
    if (checksum[i] == '1') {
      no_error = false;
      break;
    }
  }

  cout << "\nresult of CRC checksum matching: ";
  if (no_error) {
    cout << "no error detected" << endl;
  } else {
    cout << "error detected" << endl;
  }

  // removing the checksum bits
  received_string =
      received_string.substr(0, received_string.length() - checksum.length());

  // received string was organized columnwise, now we need to organize it
  // rowwise
  vector<string> received_string_rowwise;
  int rows = received_string.length() / row_length;

  for (int i = 0; i < rows; i++) {
    string temp = "";
    received_string_rowwise.push_back(temp);
  }

  for (int i = 0; i < received_string.length(); i++) {
    received_string_rowwise[i % rows] += received_string[i];
  }

  return received_string_rowwise;
}

string correct_hamming_code(string str, int m) {
  int r = number_of_redundancy_bits(m);
  int n = str.length();

  bool error_bits[r] = {false};
  // now calculate the redundancy bits
  for (int i = 0; i < r; i++) {
    int step =
        1 << i; // for r1, we go by 1 step, for r2, we go by 2 steps, etc.

    bool redundancy_bit = false;
    bool flag = false;
    for (int j = (1 << i) - 1; j < n; j += step * 2) {
      for (int k = 0; k < step; k++) {
        if (!flag) { // avoiding the parity itself
          flag = true;
          continue;
        }
        if (str[j + k] == '1') {
          redundancy_bit = !redundancy_bit;
        }
      }
    }
    // cout << "r" << i + 1 << ": " << redundancy_bit << endl;
    error_bits[i] = redundancy_bit;
  }

  int error_index = 0;
  for (int i = 0; i < r; i++) {
    int k = (1 << i) - 1;
    // now check whether the calculated redundancy bit is same as the received
    // one
    if (str[k] == '1' && !error_bits[i]) {
      error_index += (1 << i);
    } else if (str[k] == '0' && error_bits[i]) {
      error_index += (1 << i);
    }
  }

  // cout << "error index: " << error_index << endl;
  if (error_index != 0 && error_index <= n) {
    str[error_index - 1] = (str[error_index - 1] == '1') ? '0' : '1';
  }

  // now remove the redundancy bits
  string temp = "";
  int k = 0;
  for (int i = 0; i < n; i++) {
    if (i == ((1 << k) - 1)) {
      k++;
    } else {
      temp += str[i];
    }
  }

  return temp;
}

int main() {
  string str, generator_polynomial;
  int m;
  double p;

  cout << "enter data string: ";
  getline(cin, str);

  cout << "enter number of data bytes in a row <m>: ";
  cin >> m;

  cout << "enter probability <p>: ";
  cin >> p;

  cout << "enter generator polynomial: ";
  cin >> generator_polynomial;

  string padded_string = pad_string(str, m);
  cout << "\n\ndata string after padding: " << padded_string << endl;

  vector<string> data_block = split_binary(string_to_binary(padded_string), m);
  cout << "\ndata block <ascii code of m characters per row>: " << endl;
  print_binary(data_block);

  vector<string> data_block_with_redundancy_bits =
      generate_redudancy_bit_string(data_block, m);
  cout << "\ndata block after adding check bits:" << endl;
  print_redundant_bit_string(data_block_with_redundancy_bits);

  string serialized_string =
      columnwise_serialize(data_block_with_redundancy_bits);
  cout << "\ndata bits after column-wise serialization:" << endl;
  cout << serialized_string << endl;

  string checksum_serialized_string =
      add_and_print_checksum(serialized_string, generator_polynomial);

  // p is the probability of error, generate a random number between 0.0 and 1.0
  // , if it is less than p, flip the bit and add the index to the error_indexes
  // vector
  vector<int> error_indexes;
  string received_string = checksum_serialized_string;
  cout << "\nreceived frame:" << endl;
  for (int i = 0; i < received_string.length(); i++) {
    double random_number = (double)rand() / RAND_MAX;
    if (random_number < p) {
      received_string[i] = (received_string[i] == '1') ? '0' : '1';
      error_indexes.push_back(i);
      cout << "\033[31m" << received_string[i] << "\033[0m";
    } else {
      cout << received_string[i];
    }
  }

  cout << endl;

  vector<string> received_string_rowwise =
      check_and_remove_checksum(received_string, generator_polynomial,
                                data_block_with_redundancy_bits[0].length());

  cout << "\ndata block after removing CRC checksum bits:" << endl;
  int error_index = 0;
  cout << received_string_rowwise.size() << ","
       << received_string_rowwise[0].length() << endl;
  for (int i = 0; i < received_string_rowwise.size(); i++) {
    for (int j = 0; j < received_string_rowwise[i].length(); j++) {
      if (error_indexes.size() != 0 &&
          error_indexes[error_index] / received_string_rowwise.size() == j &&
          error_indexes[error_index] % received_string_rowwise.size() == i) {
        cout << "\033[31m" << received_string_rowwise[i][j] << "\033[0m";
        error_index++;
      } else {
        cout << received_string_rowwise[i][j];
      }
    }
    cout << endl;
  }
  cout << endl;

  vector<string> corrected_string_rowwise;
  for (int i = 0; i < received_string_rowwise.size(); i++) {
    corrected_string_rowwise.push_back(
        correct_hamming_code(received_string_rowwise[i], m));
  }

  cout << "\ndata block after removing check bits:" << endl;
  print_binary(corrected_string_rowwise);

  cout << "\noutput frame: ";
  for (int i = 0; i < corrected_string_rowwise.size(); i++) {
    cout << binary_to_string(corrected_string_rowwise[i]);
  }

  cout << endl;

  return 0;
}