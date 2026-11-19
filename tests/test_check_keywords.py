"""验证 Python 补全关键字与文法不一致时门禁会失败。"""

from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path
import sys
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
import check_keywords


class CheckKeywordsTests(unittest.TestCase):
    def test_missing_python_completion_keyword(self):
        keywords = check_keywords.completion_keywords()
        keywords.remove("null")
        with patch.object(check_keywords, "completion_keywords", return_value=keywords), redirect_stdout(StringIO()):
            with self.assertRaisesRegex(SystemExit, "Python 补全关键字与文法不一致：缺少 null"):
                check_keywords.main()

    def test_extra_python_completion_keyword(self):
        keywords = check_keywords.completion_keywords()
        keywords.add("testOnlyKeyword")
        with patch.object(check_keywords, "completion_keywords", return_value=keywords), redirect_stdout(StringIO()):
            with self.assertRaisesRegex(SystemExit, "Python 补全关键字与文法不一致：.*多出 testOnlyKeyword"):
                check_keywords.main()


if __name__ == "__main__":
    unittest.main()
