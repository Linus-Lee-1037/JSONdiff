# JSONdiff

## Introduction
This package is designed for analyzing differences between JSON files. I have utilized 'RapidJSON' as the parser for these files and developed my own differ algorithm.

### Definition of differences
- **Primitive Elements (string, number, bool, null):** Both type and value differences are considered as "value_changes". The element must be exactly the same, or it will be considered different.
- **Object:** A key only in the left (old) JSON object is viewed as "object:remove". Conversely, a key only in the right (new) JSON is "object::add". For keys in both JSONs, we perform recursive difference analysis.
- **Array:** There are two modes for analyzing array differences:
  - **Fast mode (default):** Arrays are compared strictly based on indices. Differences in longer arrays are reported as "array:remove" or "array:add".
  - **Advanced mode:** Uses the Longest Common Subsequence (LCS) algorithm for best array index pairs comparison. Differences are reported based on internal element comparison.
  - **Hirscheberg's algorithm:** Hirscheberg's algorithm can reduce memory consumption from 1,060 MB to 77 MB for the comparasion of two JSON files with the size of 25 MB.

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
        
        if score_ >= SIMILARITY_THRESHOLD:
            pair_list[i-1] = j-1
            i = i - 1
            j = j - 1
        else if dp[i-1][j] > dp[i][j-1]:
            i = i - 1
        else:
            j = j - 1

    return pair_list
```

For Hirscheberg's algorithm, I referred to [Hirschberg's algorithm](https://en.wikipedia.org/wiki/Hirschberg%27s_algorithm).

## Examples
For left.json and right.json, result.txt is running under the default-fast mode and result0.txt is running under the advanced mode using the default similarity threshold.
For left1.json and right1.json, result1.txt is running under the default-fast mode and result2.txt is running under the advanced mode using the default similarity threshold.
![examples](https://github.com/Linus-Lee-1037/JSONdiff/blob/main/figure/fd99d60ec6f7f70f66ed2c7e41cc05f.png)
For large-file-left.json and large-file-right.json with the size of 25 MB, the program took 77 MB and finished in 8 minutes.<br>
![examples](https://github.com/Linus-Lee-1037/JSONdiff/blob/main/figure/Large-file-result.png)

## Compilation
This work is based on a x86_64 Windows system. To compile it, you'd best install [RapidJSON](https://rapidjson.org/) first. Then refer the following shell commandline code:
```bash
g++.exe -fdiagnostics-color=always -Ofast "path\to\JsonDiff\jsondiff.cpp" "path\to\JsonDiff\src\*.cpp" -o "path\to\jsondiff.exe" "-Ipath\to\JsonDiff\include" "-Ipath\to\rapidjson\include"
```

## Use
-left "path\to\json\file": input left json or json file.<br>
-right "path\to\json\file": input right json or json file.<br>
-advanced or -A: enable the advanced mode.<br>
-hirscheberg or -H: enable the Hirscheberg algorithm (hint: you must enbale the advanced mode first).<br>

## Reference
1. [JYCM](https://github.com/eggachecat/jycm)
2. [Hirscheberg's algorithm](https://en.wikipedia.org/wiki/Hirschberg%27s_algorithm)
3. [RapidJSON](https://rapidjson.org/)
