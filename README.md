# JSONdiff

## Introduction
This package is designed for analyzing differences between JSON files. I have utilized 'RapidJSON' as the parser for these files and developed my own differ algorithm.

### Definition of differences
- **Primitive Elements (string, number, bool, null):** Both type and value differences are considered as "value_changes". The element must be exactly the same, or it will be considered different.
- **Object:** A key only in the left (old) JSON object is viewed as "object:remove". Conversely, a key only in the right (new) JSON is "object::add". For keys in both JSONs, we perform recursive difference analysis.
- **Array:** There are two modes for analyzing array differences:
  - **Fast mode (default):** Arrays are compared strictly based on indices. Differences in longer arrays are reported as "array:remove" or "array:add".
  - **Advanced mode:** Uses the Longest Common Subsequence (LCS) algorithm for best array index pairs comparison. Differences are reported based on internal element comparison.

## Algorithm
In advanced mode for array comparison, the LCS algorithm is used with a user-defined or default similarity threshold (0.5). The pseudo code for the LCS implementation is as follows:

```python
function LCS(level):
    len_left = level.left.Size()
    len_right = level.right.Size()
    
    dp = create 2D array of size (len_left + 1) x (len_right + 1)
    for i from 1 to (len_left + 1):
        dp[i] = create 1D array of size (len_right + 1) filled with 0.0

    for i from 1 to len_left:
        for j from 1 to len_right:
            level_ = create TreeLevel object with left[i-1], right[j-1], and paths
            score_ = diff_level(level_, true)
            if score_ > 0:
                dp[i][j] = dp[i-1][j-1] + score_
            else:
                dp[i][j] = max(dp[i-1][j], dp[i][j-1])

    pair_list = create an empty map
    i = len_left
    j = len_right

    while i > 0 and j > 0:
        level_ = create TreeLevel object with left[i-1], right[j-1], and paths
        score_ = diff_level(level_, true)
        
        if score_ > SIMILARITY_THRESHOLD:
            pair_list[i-1] = j-1
            i = i - 1
            j = j - 1
        else if dp[i-1][j] > dp[i][j-1]:
            i = i - 1
        else:
            j = j - 1

    return pair_list
