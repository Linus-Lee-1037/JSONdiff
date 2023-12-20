# JSONdiff
\documentclass{article}
\usepackage{graphicx} % Required for inserting images
\usepackage{listings} % Required for listings

\title{JSONdiff}
\author{Wenbo Li}
\date{December 2023}
\setlength{\parindent}{0pt}
\begin{document}

\maketitle

\section{Introduction}
This package is designed for analyzing differences between JSON files. I have utilized 'RapidJSON' as the parser for these files and developed my own differ algorithm.
\subsection{Definition of differences}

\textbf{For primitive elements of JSON like string, number, bool, and null}, both type difference and value difference are regard as the "value\_changes" using a function defined in document.cpp. The primitive element must be exactly same or it will be considered as difference.

\textbf{For object}, if a key is only contained in an object of left(old) JSON, it will be viewed as an "object:remove" because this element in doesn't exist in the object of the right(new) JSON meaning it is removed. On the contrary, a key which only exists in the object of right JSON will be regard as an "object::add" since this key value pair is not in the object of left JSON. What's more, if a key is contained in both JSON files, then we perform recursive difference analysis on its' value. In this case, the difference in its' value may not be reported as an "object::remove" or "object::add" because the analysis is happening inside of it.

\textbf{For array}, I designed two way for analyzing the difference between the left(old) and right(new) JSON.

In fast mode (our default mode), two arrays will be strictly compared one by one based on their indices. Still, elements that are not primitive will undergo recursive difference analysis and the difference between them may not be reported as an "object::remove" or "object::add" because the analysis is happening inside of it. The elements corresponding to the remaining indices of the longer array will all be reported as either "array:remove" or "array:add," depending on which array is longer, whether it's the left or right array.

In advanced mode, I use the Longest Common Subsequence (LCS) algorithm to match the best array index pairs. 
Once we have obtained all the best index pairs, we compare them in both arrays, reporting differences based on the internal element comparison. The remaining indices are all considered differences and are directly reported as "array::remove" or "array::add," depending on whether the indices are from the left JSON or the right JSON.

\section{Algorithm}
In the advanced mode for array comparison, based on the user-defined similarity threshold or the default threshold (0.5), the LCS algorithm compares each element in the left JSON and right JSON arrays one by one (note: at this point, the comparison results only return scores and do not report differences). If the score obtained from the comparison is higher than the similarity threshold, the current position dp[i][j] will have a score equal to dp[i-1][j-1] plus the comparison score; otherwise, the score at dp[i][j] will be the maximum of dp[i][j-1] and dp[i-1][j]. 
During the backtracking phase, based on the indices corresponding to dp[i-1][j-1], we compare the elements of the two arrays (still, the comparison results only return scores and do not report differences). If the comparison score is higher than the threshold, we record this pair of indices. In other cases, the process aligns with the traditional LCS algorithm: we observe dp[i-1][j] and dp[i][j-1] to determine which direction to continue backtracking, repeating the previous steps. The pseudo code is given:
\begin{lstlisting}[language=Python, caption={Longest Common Subsequence (LCS)}]
function LCS(level):
    len_left = level.left.Size()
    len_right = level.right.Size()
    
    dp = create 2D array of size (len_left + 1)
            x (len_right + 1)
    for i from 1 to (len_left + 1):
        dp[i] = create 1D array of size (
            len_right + 1) filled with 0.0

    for i from 1 to len_left:
        for j from 1 to len_right:
            level_ = create TreeLevel object with
                     left[i-1], right[j-1], and paths
            score_ = diff_level(level_, true)
            if score_ > 0:
                dp[i][j] = dp[i-1][j-1] + score_
            else:
                dp[i][j] = max(dp[i-1][j], dp[i][j-1])

    pair_list = create an empty map
    i = len_left
    j = len_right

    while i > 0 and j > 0:
        level_ = create TreeLevel object with
                 left[i-1], right[j-1], and paths
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
\end{lstlisting}
\section{Implementation}
All classes and functions are encapsulated within the 'Linus::jsondiff' namespace in document.h and document.cpp, which helps to avoid naming conflicts with other parts of a program that might use similar names. An additional jsondiff.cpp is used for command-line invocation, which takes parameters and produces output results.

\subsection{'ValueToString' Function}
Converts a RapidJSON 'Value' to a 'std::string'. This utility function is used to serialize JSON values into strings for comparison and reporting.

\subsection{'KeysFromObject' Function}
Takes a JSON object represented by a RapidJSON 'Value' and returns a 'std::vector<std::string>' containing all keys in the object. It's used to iterate over keys in a JSON object for comparison purposes.

\subsection{'TreeLevel' Class}
Represents a level in the JSON tree during comparison. It holds two JSON values, one from each JSON tree being compared, and paths that describe the location of these values in their respective trees.

\subsubsection{Constructor 'TreeLevel'}
Initializes a TreeLevel object with JSON values and their paths.

\subsubsection{'get\_type' Member Function}
Determines the type of JSON values (Object, Array, String, Number, Bool, Null, or TypeMismatch) for comparison purposes.

\subsubsection{'to\_info' Member Function}
Serializes the 'TreeLevel' information into a JSON-like string format for reporting.

\subsubsection{'get\_key' Member Function}
Constructs a key that uniquely identifies the TreeLevel based on its paths in the JSON trees.

\subsection{'JsonDiffer' Class}
The core class that performs the comparison between two JSON values.

\subsubsection{Constructor 'JsonDiffer'}
Sets up the JsonDiffer with two JSON values, a mode for comparison (advanced or not), and a similarity threshold for array comparisons.

\subsubsection{'report' Member Function}
Records an event (like 'add', 'remove', or 'change') during comparison, along with the 'TreeLevel' where it occurred.

\subsubsection{'to\_info' Member Function}
Returns a map with event types as keys and vectors of stringified JSON differences as values.

\subsubsection{'compare\_array' Member Function}
Compares two JSON arrays, with an option for a detailed or fast comparison, and returns a similarity score.

\subsubsection{'compare\_array\_fast' Member Function}
A faster but less detailed array comparison method that iterates over the arrays in parallel and compares corresponding elements.

\subsubsection{'compare\_array\_advanced' Member Function}
A more detailed and potentially slower method that uses the Longest Common Subsequence (LCS) algorithm to compare arrays.

\subsubsection{'LCS' Member Function}
Implements the Longest Common Subsequence algorithm to find the best matching pairs of elements between two arrays, used by 'compare\_array\_advanced'.

\subsubsection{'compare\_object' Member Function}
Compares two JSON objects, identifying added, removed, or changed keys and values.

\subsubsection{'compare\_primitive' Member Function}
Compares two primitive JSON values (string, number, bool, null) and determines if they are equal or not.

\subsubsection{'\_diff\_level' Private Member Function}
Internal function that dispatches the comparison to the appropriate method based on the types of the JSON values.

\subsubsection{'diff\_level' Member Function}
Caches the results of comparisons and returns a similarity score for a given 'TreeLevel'.

\subsubsection{'diff' Member Function}
Initiates the comparison process from the root level of the JSON values and determines if they are identical.

\section{How to use}
The program can be invoked directly from the command line using the command `jsonsdiff -left "path to your left JSON" -right "path to your right JSON"` or just put your JSON content behind the '-left' and '-right'. The `-advanced` or `-A` option allows you to enable the LCS algorithm for array comparison, and the `-similarity` or `-S` followed by a double value allows you to customize the similarity threshold. All the source code and binary files can be found on my Github.

\end{document}
