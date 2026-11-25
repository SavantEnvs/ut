      # Comments start with # at the beginning of a line
      @calculator
      Feature: Calculator

        Background:
          Given I have calculator

        Scenario Outline: Addition of <a> and <b>
          When I enter <a>
          But I enter <b>
          * I press add
          Then I expect <sum>

          Examples:
            | a  | b | sum |
            | 1  | 2 | 3   |
            | 40 | 2 | 42  |

        Scenario: Sum of a table
          When I enter the following values:
            | 1 | 2 | 3 |
            | 4 | 5 | 6 |
           And I press add
          Then I expect 21

        Rule: Subtraction

          Scenario: Subtraction of a doc string
            When I enter the numbers:
              """
              45 2 1
              """
            And I press sub
            Then I expect 42

          @skip
          Scenario: Not implemented yet
            When I enter 1
            Then I expect 2
