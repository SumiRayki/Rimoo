package com.yuyan.imemodule.keyboard.container

import android.annotation.SuppressLint
import android.content.Context
import android.view.LayoutInflater
import android.view.MotionEvent
import android.view.View
import androidx.constraintlayout.widget.ConstraintLayout
import com.yuyan.imemodule.R
import com.yuyan.imemodule.prefs.AppPrefs
import com.yuyan.imemodule.singleton.EnvironmentSingleton
import com.yuyan.imemodule.utils.KeyboardLoaderUtil
import com.yuyan.imemodule.keyboard.InputView
import com.yuyan.imemodule.keyboard.KeyboardManager
import com.yuyan.imemodule.view.preference.ManagedPreference
import splitties.dimensions.dp
import splitties.views.bottomPadding
import splitties.views.rightPadding
import kotlin.math.abs
import kotlin.math.absoluteValue

/**
 * 软键盘View集装箱
 * 所有软键盘（输入、符号、设置等）父容器View。
 */
@SuppressLint("ViewConstructor")
open class BaseContainer(@JvmField var mContext: Context, inputView: InputView) : ConstraintLayout(mContext) {
    //输入法服务
    @JvmField
    protected var inputView: InputView
    private lateinit var mRightPaddingKey: ManagedPreference.PInt
    private lateinit var mBottomPaddingKey: ManagedPreference.PInt
    private lateinit var mWidthRatioKey: ManagedPreference.PFloat
    private var keyboardResizeOverlay: View? = null

    /**
     * 更新软键盘布局
     */
    open fun updateSkbLayout(){
    }

    init {
        this.inputView = inputView
    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        val measuredWidth = EnvironmentSingleton.instance.skbWidth
        val measuredHeight = EnvironmentSingleton.instance.skbHeight
        val widthMeasure = MeasureSpec.makeMeasureSpec(measuredWidth, MeasureSpec.EXACTLY)
        val heightMeasure = MeasureSpec.makeMeasureSpec(measuredHeight, MeasureSpec.EXACTLY)
        super.onMeasure(widthMeasure, heightMeasure)
    }

    /**
     * 设置键盘高度
     */
    @SuppressLint("ClickableViewAccessibility")
    fun setKeyboardHeight() {
        val env = EnvironmentSingleton.instance
        keyboardResizeOverlay?.let { removeView(it) }
        val rootView = LayoutInflater.from(context).inflate(R.layout.layout_ime_keyboard_height_shadow, this, false)
        keyboardResizeOverlay = rootView
        this.addView(rootView)
        rootView.findViewById<View>(R.id.ll_keyboard_height_reset).setOnClickListener { _: View? ->
            EnvironmentSingleton.instance.keyBoardHeightRatio = 0.3f
            EnvironmentSingleton.instance.keyBoardWidthRatio = when {
                EnvironmentSingleton.instance.keyboardModeFloat -> 1f
                EnvironmentSingleton.instance.isLandscape -> 0.7f
                else -> 1f
            }
            EnvironmentSingleton.instance.initData()
            KeyboardLoaderUtil.instance.clearKeyboardMap()
            KeyboardManager.instance.clearKeyboard()
            updateSkbLayout()
            rootView.layoutParams = LayoutParams(
                EnvironmentSingleton.instance.skbWidth,
                EnvironmentSingleton.instance.skbHeight
            )
        }
        rootView.findViewById<View>(R.id.ll_keyboard_height_sure).setOnClickListener {
            removeView(rootView)
            keyboardResizeOverlay = null
        }
        rootView.findViewById<View>(R.id.iv_keyboard_height_Top).setOnTouchListener { v12: View, event -> onModifyKeyboardHeightEvent(v12, event) }
        rootView.findViewById<View>(R.id.iv_keyboard_width_left).setOnTouchListener { v12: View, event -> onModifyKeyboardWidthEvent(v12, event, true) }
        rootView.findViewById<View>(R.id.iv_keyboard_width_right).setOnTouchListener { v12: View, event -> onModifyKeyboardWidthEvent(v12, event, false) }
        if(env.keyboardModeFloat){
            mBottomPaddingKey = if(env.isLandscape) AppPrefs.getInstance().internal.keyboardBottomPaddingLandscapeFloat
            else AppPrefs.getInstance().internal.keyboardBottomPaddingFloat
            mRightPaddingKey = if(env.isLandscape) AppPrefs.getInstance().internal.keyboardRightPaddingLandscapeFloat
            else AppPrefs.getInstance().internal.keyboardRightPaddingFloat
            mWidthRatioKey = if(env.isLandscape) AppPrefs.getInstance().internal.keyboardWidthRatioLandscapeFloat
            else AppPrefs.getInstance().internal.keyboardWidthRatioFloat
        } else {
            mBottomPaddingKey = AppPrefs.getInstance().internal.keyboardBottomPadding
            mRightPaddingKey = AppPrefs.getInstance().internal.keyboardRightPadding
            mWidthRatioKey = if(env.isLandscape) AppPrefs.getInstance().internal.keyboardWidthRatioLandscape
            else AppPrefs.getInstance().internal.keyboardWidthRatio
        }
        rootView.layoutParams = LayoutParams(env.skbWidth, env.skbHeight)
        rootView.findViewById<View>(R.id.iv_keyboard_move).setOnTouchListener { _, event -> onMoveKeyboardEvent(event) }
    }

    private val lastY = floatArrayOf(0f)
    private val lastX = floatArrayOf(0f)
    var isHandling = false
    private fun onModifyKeyboardHeightEvent(v12: View, event: MotionEvent): Boolean {
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> lastY[0] = event.y
            MotionEvent.ACTION_MOVE -> {
                val y = event.y
                if (!isHandling && abs((y - lastY[0]).toDouble()) > dp(10)) {
                    isHandling = true
                    var rat = EnvironmentSingleton.instance.keyBoardHeightRatio
                    if (y < lastY[0]) { // 手指向上移动
                        rat += 0.01f
                    } else { // 向下移动
                        rat -= 0.01f
                    }
                    lastY[0] = y
                    EnvironmentSingleton.instance.keyBoardHeightRatio = rat
                    EnvironmentSingleton.instance.initData()
                    KeyboardLoaderUtil.instance.clearKeyboardMap()
                    KeyboardManager.instance.clearKeyboard()
                    updateSkbLayout()
                    val l = LayoutParams(
                        EnvironmentSingleton.instance.skbWidth,
                        EnvironmentSingleton.instance.skbHeight
                    )
                    keyboardResizeOverlay?.layoutParams = l
                    isHandling = false
                }
            }
            MotionEvent.ACTION_UP -> v12.performClick()
        }
        return true
    }

    private fun onModifyKeyboardWidthEvent(v12: View, event: MotionEvent, isLeftHandle: Boolean): Boolean {
        when (event.actionMasked) {
            MotionEvent.ACTION_DOWN -> lastX[0] = event.rawX
            MotionEvent.ACTION_MOVE -> {
                val x = event.rawX
                if (!isHandling && abs((x - lastX[0]).toDouble()) > dp(10)) {
                    isHandling = true
                    val previousWidth = EnvironmentSingleton.instance.skbWidth
                    var ratio = EnvironmentSingleton.instance.keyBoardWidthRatio
                    val enlarge = if (isLeftHandle) x < lastX[0] else x > lastX[0]
                    ratio = if (enlarge) ratio + 0.02f else ratio - 0.02f
                    ratio = ratio.coerceIn(0.55f, 1f)
                    lastX[0] = x
                    EnvironmentSingleton.instance.keyBoardWidthRatio = ratio
                    mWidthRatioKey.setValue(ratio)
                    EnvironmentSingleton.instance.initData()
                    if (!EnvironmentSingleton.instance.keyboardModeFloat && !isLeftHandle) {
                        val deltaWidth = EnvironmentSingleton.instance.skbWidth - previousWidth
                        val maxRightGap = (EnvironmentSingleton.instance.inputAreaWidth - EnvironmentSingleton.instance.skbWidth).coerceAtLeast(0)
                        val adjustedRightGap = (mRightPaddingKey.getValue() - deltaWidth).coerceIn(0, maxRightGap)
                        mRightPaddingKey.setValue(adjustedRightGap)
                    }
                    KeyboardLoaderUtil.instance.clearKeyboardMap()
                    KeyboardManager.instance.clearKeyboard()
                    updateSkbLayout()
                    keyboardResizeOverlay?.layoutParams = LayoutParams(
                        EnvironmentSingleton.instance.skbWidth,
                        EnvironmentSingleton.instance.skbHeight
                    )
                    isHandling = false
                }
            }
            MotionEvent.ACTION_UP -> v12.performClick()
        }
        return true
    }

    private var initialTouchX = 0f
    private var initialTouchY = 0f
    private var rightPaddingValue = 0  // 右侧边距
    private var bottomPaddingValue = 0  // 底部边距
    private var mSkbRootHeight = 0  // 键盘高度
    private var mSkbRootWidth = 0  // 键盘宽度
    private fun onMoveKeyboardEvent(event: MotionEvent?): Boolean {
        when (event?.action) {
            MotionEvent.ACTION_DOWN -> {
                bottomPaddingValue = mBottomPaddingKey.getValue()
                rightPaddingValue = mRightPaddingKey.getValue()
                initialTouchX = event.rawX
                initialTouchY = event.rawY
                mSkbRootHeight = inputView.mSkbRoot.height
                mSkbRootWidth = inputView.mSkbRoot.width
                return true
            }
            MotionEvent.ACTION_MOVE -> {
                val dx: Float = event.rawX - initialTouchX
                val dy: Float = event.rawY - initialTouchY
                if(dx.absoluteValue > 10) {
                    rightPaddingValue -= dx.toInt()
                    rightPaddingValue = if(rightPaddingValue < 0) 0
                    else if(rightPaddingValue > inputView.width - mSkbRootWidth) {
                        inputView.width - mSkbRootWidth
                    } else rightPaddingValue
                    initialTouchX = event.rawX
                    if(EnvironmentSingleton.instance.keyboardModeFloat) {
                        inputView.rightPadding = rightPaddingValue
                    } else {
                        inputView.mSkbRoot.rightPadding = rightPaddingValue
                    }
                }
                if(dy.absoluteValue > 10 ) {
                    bottomPaddingValue -= dy.toInt()
                    bottomPaddingValue = if(bottomPaddingValue < 0) 0
                    else if(bottomPaddingValue > inputView.height - mSkbRootHeight) {
                        inputView.height - mSkbRootHeight
                    } else bottomPaddingValue
                    initialTouchY = event.rawY
                    if(EnvironmentSingleton.instance.keyboardModeFloat) {
                        inputView.bottomPadding = bottomPaddingValue
                    } else {
                        inputView.mSkbRoot.bottomPadding = bottomPaddingValue
                    }
                }
                return true
            }
            MotionEvent.ACTION_UP, MotionEvent.ACTION_CANCEL -> {
                mRightPaddingKey.setValue(rightPaddingValue)
                mBottomPaddingKey.setValue(bottomPaddingValue)
            }
        }
        return false
    }
}
