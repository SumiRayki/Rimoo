package com.yuyan.imemodule.database

import com.yuyan.imemodule.application.Launcher
import com.yuyan.imemodule.database.entry.Phrase
import com.yuyan.imemodule.libs.pinyin4j.PinyinHelper
import com.yuyan.imemodule.libs.pinyin4j.format.HanyuPinyinCaseType
import com.yuyan.imemodule.libs.pinyin4j.format.HanyuPinyinOutputFormat
import com.yuyan.imemodule.libs.pinyin4j.format.HanyuPinyinToneType
import com.yuyan.imemodule.libs.pinyin4j.format.HanyuPinyinVCharType
import com.yuyan.inputmethod.util.LX17PinYinUtils
import com.yuyan.inputmethod.util.T9PinYinUtils

object BundledPhraseLoader {
    private const val ASSET_PATH = "phrases/mainland_hot_phrases.txt"

    fun load(): List<Phrase> {
        val format = HanyuPinyinOutputFormat().apply {
            setCaseType(HanyuPinyinCaseType.LOWERCASE)
            setToneType(HanyuPinyinToneType.WITHOUT_TONE)
            setVCharType(HanyuPinyinVCharType.WITH_V)
        }
        return Launcher.instance.context.assets.open(ASSET_PATH).bufferedReader(Charsets.UTF_8).useLines { lines ->
            lines
                .map { it.trim() }
                .filter { it.isNotEmpty() && !it.startsWith("#") }
                .distinct()
                .mapNotNull { buildPhrase(it, format) }
                .toList()
        }
    }

    private fun buildPhrase(content: String, format: HanyuPinyinOutputFormat): Phrase? {
        val qwerty = PinyinHelper.toHanYuPinyin(content, format, "")
            .lowercase()
            .filter { it in 'a'..'z' }
        if (qwerty.isBlank()) return null
        val t9 = qwerty.map { T9PinYinUtils.pinyin2T9Key(it) }.joinToString("")
        val lx17 = qwerty.map { LX17PinYinUtils.pinyin2Lx17Key(it) }.joinToString("")
        return Phrase(content = content, t9 = t9, qwerty = qwerty, lx17 = lx17, time = 0L)
    }
}
