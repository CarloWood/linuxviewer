#pragma once

enum gender_type
{
  gender_female,
  gender_male
};

class Gender
{
 private:
  gender_type m_gender;

 public:
  Gender() : m_gender(gender_female) { }

  void set_gender(gender_type gender) { m_gender = gender; }
  bool is_female() const { return m_gender == gender_female; }
  bool is_male() const { return m_gender == gender_male; }
};
