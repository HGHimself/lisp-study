(defmacro with-gensym ((&rest names) &body body)
  `(let ,(loop for n in names collect `(,n (gensym)))
     ,@body))

(defun test-+ ()
   (format t "~:[FAIL~;pass~] ... ~a~%" (= (+ 1 2) 3) '(= (+ 1 2) 3))
   (format t "~:[FAIL~;pass~] ... ~a~%" (= (+ 4 2) 6) '(= (+ 4 2) 6))
   (format t "~:[FAIL~;pass~] ... ~a~%" (= (+ -4 -4) -8) '(= (+ -4 -4) -8)))

(defun report-result (result form)
  (format t "~:[FAIL~;pass~] ... ~a~%" result form)
  result)

(defmacro check (&body forms)
  `(combine-results
     ,@(loop for f in forms collect `(report-result ,f ',f))))

(defun test-+ ()
  (check
    (= (+ 1 2) 3)
    (= (+ 4 3) 5)
    (= (+ -4 -4) -8)))

(defmacro combine-results (&body results)
  (with-gensyms (result)
    `(let ((,result t))
       ,@(loop for r in results collect `(unless ,r (setf ,result nil)))
       ,result)))

(defvar *test-var* nil)
